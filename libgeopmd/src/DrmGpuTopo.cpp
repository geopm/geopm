/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmGpuTopo.hpp"

#include <dirent.h>
#include <errno.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_topo.h"

static const int MAX_CPUS_PER_CPUMASK_SEGMENT = 32;
static const std::regex GPU_CARD_REGEX("^card(\\d+)$");
static const std::regex GPU_TILE_REGEX("^gt(\\d+)$");

static std::set<int> linux_cpumask_buf_to_int_set(const std::string &cpumask_buf)
{
    // The expected bitmask format is "HEX,HEX,...,HEX", where commas separate
    // 32-bit segments. Higher-ordered bits indicate higher CPU indices (i.e.
    // LSB is CPU 0).
    std::set<int> mapped_cpus;
    int cpu_offset = 0;
    auto hex_segments = geopm::string_split(cpumask_buf, ",");
    for (auto it = hex_segments.rbegin(); it != hex_segments.rend(); ++it) {
        auto bitmask_segment = std::stoull(*it, nullptr, 16);
        if (bitmask_segment >> MAX_CPUS_PER_CPUMASK_SEGMENT) {
            throw geopm::Exception("linux_cpumask_buf_to_int_set: malformed cpumask: " + cpumask_buf,
                                   GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int next_segment_cpu_offset = cpu_offset + MAX_CPUS_PER_CPUMASK_SEGMENT;
        while (cpu_offset < next_segment_cpu_offset) {
            if (bitmask_segment & 1) {
                mapped_cpus.insert(cpu_offset);
            }
            bitmask_segment >>= 1;
            cpu_offset += 1;
        }
    }
    return mapped_cpus;
}

// Return the name of the driver that provides the given /sys/class/drm/card*/ device
static std::string drm_driver_name_from_card_path(const std::string &card_path)
{
    auto driver_path = geopm::read_symlink_target(card_path + "/device/driver");
    return driver_path.substr(driver_path.find_last_of("/") + 1);
}

static std::vector<std::string> get_file_paths_with_pattern(
    const std::string &parent_directory_path,
    const std::regex &pattern)
{
    std::vector<std::string> paths;
    std::vector<std::string> directory_files;
    try {
        directory_files = geopm::list_directory_files(parent_directory_path);
    }
    catch (const geopm::Exception &e) {
    }

    for (const auto &child_file : directory_files) {
        std::smatch file_match;
        if (!std::regex_match(child_file, file_match, pattern)) {
            continue;
        }

        if (file_match.size() != 2) {
            // There should be two matches: the whole regex and one capture group
            throw std::logic_error(std::string(__func__) + ": Expected 2 matching groups, found " +
                                   std::to_string(file_match.size()));
        }
        paths.emplace_back(parent_directory_path + "/" + child_file);
    }
    return paths;
}

// Query which driver provides each card. Return the pair of (driver name) and
// (vector of card drm paths) of the driver with the most cards present.
using DriverName = std::string;
using CardVector = std::vector<std::string>;
using DriverCards = std::pair<DriverName, CardVector>;
static DriverCards get_cards_from_most_frequent_driver(const CardVector &all_cards)
{
    std::map<DriverName, CardVector> driver_card_paths; // Map of (driver name)->(vector of card paths)
    for (const auto &card_path : all_cards) {
        auto driver_name = drm_driver_name_from_card_path(card_path);
        driver_card_paths[driver_name].push_back(card_path);
    }
    auto driver_with_max_cards_it = std::max_element(
        driver_card_paths.begin(), driver_card_paths.end(),
        [](const DriverCards &lhs, const DriverCards &rhs) {
            return lhs.second.size() < rhs.second.size();
        });
    if (driver_with_max_cards_it == driver_card_paths.end()) {
        // This should only happen if driver_card_paths (and all_cards) are empty.
        throw std::logic_error(std::string(__func__) + ": No max-count is defined for driver card paths");
    }
    return *driver_with_max_cards_it;
}

namespace geopm
{
    DrmGpuTopo::DrmGpuTopo(const std::string &drm_directory)
    {
        std::vector<std::string> drm_card_paths =
            get_file_paths_with_pattern(drm_directory, GPU_CARD_REGEX);
        // Ensure that the GPU list is ordered since we'll be accumulating
        // global gpu_chip indices from this list next.
        std::sort(drm_card_paths.begin(), drm_card_paths.end());

        if (!drm_card_paths.empty()) {
            auto driver_cards = get_cards_from_most_frequent_driver(drm_card_paths);
            m_driver_name = driver_cards.first;
            drm_card_paths = driver_cards.second;
        }
        else {
            throw Exception("DrmGpuTopo::" + std::string(__func__) +
                            ": No supported drm cards are detected",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        int tiles_per_card = -1;
        for (size_t gpu_idx = 0; gpu_idx < drm_card_paths.size(); ++gpu_idx) {
            const auto &card_path = drm_card_paths[gpu_idx];
            m_card_paths.push_back(card_path);
            std::vector<std::string> tile_paths_in_card =
                get_file_paths_with_pattern(card_path + "/gt", GPU_TILE_REGEX);
            if (tiles_per_card == -1) {
                tiles_per_card = tile_paths_in_card.size();
            }
            else {
                if (tiles_per_card != static_cast<int>(tile_paths_in_card.size())) {
                    std::ostringstream oss;
                    oss << "DrmGpuTopo::" << __func__ << ": Mixed counts of "
                        << "gpu_chip per gpu are not supported. Encountered at "
                        << "least one gpu with " << tiles_per_card << " tiles "
                        << "per card and " << tile_paths_in_card.size()
                        << " tiles on " << card_path;
                    throw Exception(oss.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            std::sort(tile_paths_in_card.begin(), tile_paths_in_card.end());
            for (const auto &tile_path : tile_paths_in_card) {
                m_gt_paths.push_back(tile_path);
                m_gpu_by_gpu_chip.push_back(gpu_idx);
            }
        }

        m_cpu_affinity_by_gpu.reserve(m_card_paths.size());
        for (const auto &card_path : m_card_paths) {
            auto cpumask_buf = geopm::read_file(card_path + "/device/local_cpus");
            m_cpu_affinity_by_gpu.push_back(linux_cpumask_buf_to_int_set(cpumask_buf));
        }
    }

    int DrmGpuTopo::num_gpu() const
    {
        return num_gpu(GEOPM_DOMAIN_GPU);
    }

    int DrmGpuTopo::num_gpu(int domain) const
    {
        int result = -1;
        if (domain == GEOPM_DOMAIN_GPU) {
            result = m_cpu_affinity_by_gpu.size();
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            result = m_gpu_by_gpu_chip.size();
        }
        else {
            throw Exception("DrmGpuTopo::" + std::string(__func__) + ": domain " +
                                std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::set<int> DrmGpuTopo::cpu_affinity_ideal(int gpu_idx) const
    {
        return cpu_affinity_ideal(GEOPM_DOMAIN_GPU, gpu_idx);
    }

    std::set<int> DrmGpuTopo::cpu_affinity_ideal(int domain, int idx) const
    {
        std::set<int> result = {};
        if (domain == GEOPM_DOMAIN_GPU) {
            if (idx < 0 || idx >= static_cast<int>(m_cpu_affinity_by_gpu.size())) {
                throw Exception("DrmGpuTopo::" + std::string(__func__) + ": idx " +
                                std::to_string(idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_by_gpu[idx];
        }
        else if (domain == GEOPM_DOMAIN_GPU_CHIP) {
            if (idx < 0 || idx >= static_cast<int>(m_gpu_by_gpu_chip.size())) {
                throw Exception("DrmGpuTopo::" + std::string(__func__) + ": idx " +
                                    std::to_string(idx) + " is out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = m_cpu_affinity_by_gpu[m_gpu_by_gpu_chip[idx]];
        }
        else {
            throw Exception("DrmGpuTopo::" + std::string(__func__) + ": domain " +
                                std::to_string(domain) + " is not supported.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    std::string DrmGpuTopo::gt_path(int gpu_chip_idx) const
    {
        if (gpu_chip_idx < 0 || gpu_chip_idx >= static_cast<int>(m_gt_paths.size())) {
            throw Exception("DrmGpuTopo::" + std::string(__func__) + ": idx " +
                                std::to_string(gpu_chip_idx) + " is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_gt_paths[gpu_chip_idx];
    }

    std::string DrmGpuTopo::card_path(int gpu_idx) const
    {
        if (gpu_idx < 0 || gpu_idx >= static_cast<int>(m_card_paths.size())) {
            throw Exception("DrmGpuTopo::" + std::string(__func__) + ": idx " +
                                std::to_string(gpu_idx) + " is out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_card_paths[gpu_idx];
    }

    std::string DrmGpuTopo::driver_name() const
    {
        return m_driver_name;
    }
}
