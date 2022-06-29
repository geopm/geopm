/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "SSTFrequencyLimitDetector.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    enum Priorities_e {
        HIGH_PRIORITY = 0,
        MEDIUM_HIGH_PRIORITY = 1,
        MEDIUM_LOW_PRIORITY = 2,
        LOW_PRIORITY = 3,
    };

    SSTFrequencyLimitDetector::SSTFrequencyLimitDetector(
        PlatformIO &platform_io,
        const PlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_package_count(platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , m_core_count(platform_topo.num_domain(GEOPM_DOMAIN_CORE))
        , m_clos_association_signals()
        , m_frequency_limit_signals()
        , m_sst_tf_enable_signals()
        , M_CPU_FREQUENCY_MAX(m_platform_io.read_signal(
              "CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        , M_CPU_FREQUENCY_STICKER(m_platform_io.read_signal(
              "CPU_FREQUENCY_STICKER", GEOPM_DOMAIN_BOARD, 0))
        , M_CPU_FREQUENCY_STEP(m_platform_io.read_signal(
              "CPU_FREQUENCY_STEP", GEOPM_DOMAIN_BOARD, 0))
        , M_ALL_CORE_TURBO_FREQUENCY(m_platform_io.read_signal(
              "MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_7", GEOPM_DOMAIN_BOARD, 0))
        , m_bucket_hp_cores{
              static_cast<unsigned int>(m_platform_io.read_signal(
                  "SST::HIGHPRIORITY_NCORES:0", GEOPM_DOMAIN_BOARD, 0)),
              static_cast<unsigned int>(m_platform_io.read_signal(
                  "SST::HIGHPRIORITY_NCORES:1", GEOPM_DOMAIN_BOARD, 0)),
              static_cast<unsigned int>(m_platform_io.read_signal(
                  "SST::HIGHPRIORITY_NCORES:2", GEOPM_DOMAIN_BOARD, 0))}
        , M_LOW_PRIORITY_SSE_FREQUENCY(m_platform_io.read_signal(
                    "SST::LOWPRIORITY_FREQUENCY:SSE", GEOPM_DOMAIN_BOARD, 0))
        , M_LOW_PRIORITY_AVX2_FREQUENCY(m_platform_io.read_signal(
                    "SST::LOWPRIORITY_FREQUENCY:AVX2", GEOPM_DOMAIN_BOARD, 0))
        , M_LOW_PRIORITY_AVX512_FREQUENCY(m_platform_io.read_signal(
                    "SST::LOWPRIORITY_FREQUENCY:AVX512", GEOPM_DOMAIN_BOARD, 0))
        , m_bucket_sse_frequency{
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_SSE:0", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_SSE:1", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_SSE:2", GEOPM_DOMAIN_BOARD, 0)}
        , m_bucket_avx2_frequency{
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX2:0", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX2:1", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX2:2", GEOPM_DOMAIN_BOARD, 0)}
        , m_bucket_avx512_frequency{
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX512:0", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX512:1", GEOPM_DOMAIN_BOARD, 0),
            m_platform_io.read_signal(
                    "SST::HIGHPRIORITY_FREQUENCY_AVX512:2", GEOPM_DOMAIN_BOARD, 0)}
        , m_sse_hp_tradeoffs()
        , m_avx2_hp_tradeoffs()
        , m_avx512_hp_tradeoffs()
        , m_cores_in_packages()
        , m_core_frequency_limits(m_core_count, {{m_core_count / m_package_count, M_CPU_FREQUENCY_MAX}})
        , m_core_lp_frequencies(m_core_count, M_CPU_FREQUENCY_STICKER)
    {
        for (size_t i = 0; i < m_core_count; ++i) {
            m_clos_association_signals.push_back(m_platform_io.push_signal(
                "SST::COREPRIORITY:ASSOCIATION", GEOPM_DOMAIN_CORE, i));
            m_frequency_limit_signals.push_back(m_platform_io.push_signal(
                "CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_CORE, i));
        }

        for (unsigned int i = 0; i < m_package_count; ++i) {
            m_sst_tf_enable_signals.push_back(m_platform_io.push_signal(
                "SST::TURBO_ENABLE:ENABLE", GEOPM_DOMAIN_PACKAGE, i));
            const auto nested_ctls = platform_topo.domain_nested(
                GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE, i);
            m_cores_in_packages.push_back(
                std::vector<int>(nested_ctls.begin(), nested_ctls.end()));
        }

        for (size_t bucket = 0; bucket < m_bucket_hp_cores.size(); ++bucket) {
            m_sse_hp_tradeoffs.emplace_back(
                m_bucket_hp_cores[bucket], m_bucket_sse_frequency[bucket]);
            m_avx2_hp_tradeoffs.emplace_back(
                m_bucket_hp_cores[bucket], m_bucket_avx2_frequency[bucket]);
            m_avx512_hp_tradeoffs.emplace_back(
                m_bucket_hp_cores[bucket], m_bucket_avx512_frequency[bucket]);
        }
    }

    void SSTFrequencyLimitDetector::update_max_frequency_estimates(
        const std::vector<double> &observed_core_frequencies)
    {
        for (size_t package_idx = 0; package_idx < m_package_count; ++package_idx) {
            const auto &cores_in_package = m_cores_in_packages[package_idx];
            if (m_platform_io.sample(m_sst_tf_enable_signals[package_idx])) {
                auto hp_core_count = std::count_if(
                    cores_in_package.begin(), cores_in_package.end(),
                    [this](int idx) {
                        return m_platform_io.sample(m_clos_association_signals[idx]) <= MEDIUM_HIGH_PRIORITY;
                    });

                double sse_freq, avx2_freq, avx512_freq;

                // Buckets are ordered from smallest to largest. Find the smallest
                // bucket that contains the configured HP core count.
                const auto bucket_it = std::find_if(
                    m_bucket_hp_cores.begin(), m_bucket_hp_cores.end(),
                    [hp_core_count](unsigned int bucket_core_count) {
                        return hp_core_count <= bucket_core_count;
                    });
                if (bucket_it == m_bucket_hp_cores.end()) {
                    sse_freq = M_ALL_CORE_TURBO_FREQUENCY;
                    // AVX2 and AVX512 limits may or may not be different. Those
                    // cannot be queried from the CPU, and are typically
                    // empirically measured if they are actually needed. Let's just
                    // approximate them for now.
                    avx2_freq = M_ALL_CORE_TURBO_FREQUENCY;
                    avx512_freq = M_ALL_CORE_TURBO_FREQUENCY;
                }
                else {
                    auto bucket_idx = std::distance(m_bucket_hp_cores.begin(), bucket_it);
                    sse_freq = m_bucket_sse_frequency[bucket_idx];
                    avx2_freq = m_bucket_avx2_frequency[bucket_idx];
                    avx512_freq = m_bucket_avx512_frequency[bucket_idx];
                }

                for (const auto core_idx : cores_in_package) {
                    auto core_frequency_limit = m_platform_io.sample(m_frequency_limit_signals[core_idx]);

                    // Two neighboring bins in the SST-TF table might or might
                    // not be equal, so check both.
                    if (observed_core_frequencies[core_idx] > avx2_freq ||
                        observed_core_frequencies[core_idx] >= sse_freq ||
                        (core_frequency_limit <= avx2_freq &&
                         is_frequency_near_limit(observed_core_frequencies[core_idx], core_frequency_limit))) {
                        m_core_frequency_limits[core_idx] = m_sse_hp_tradeoffs;
                        m_core_lp_frequencies[core_idx] = M_LOW_PRIORITY_SSE_FREQUENCY;
                    }
                    else if (observed_core_frequencies[core_idx] > avx512_freq ||
                             observed_core_frequencies[core_idx] >= avx2_freq ||
                             (core_frequency_limit <= avx512_freq &&
                              is_frequency_near_limit(observed_core_frequencies[core_idx], core_frequency_limit))) {
                        m_core_frequency_limits[core_idx] = m_avx2_hp_tradeoffs;
                        m_core_lp_frequencies[core_idx] = M_LOW_PRIORITY_AVX2_FREQUENCY;
                    }
                    else {
                        m_core_frequency_limits[core_idx] = m_avx512_hp_tradeoffs;
                        m_core_lp_frequencies[core_idx] = M_LOW_PRIORITY_AVX512_FREQUENCY;
                    }
                }
            }
            else {
                // SST-TF is available, but disabled. Assume any core can reach
                // the current max observed frequency across cores.
                const auto core_with_max_frequency = *std::max_element(
                    cores_in_package.begin(), cores_in_package.end(),
                    [&observed_core_frequencies](int lhs, int rhs) {
                        return observed_core_frequencies[lhs] < observed_core_frequencies[rhs];
                    });
                const auto max_frequency = observed_core_frequencies[core_with_max_frequency];
                for (const auto core_idx : cores_in_package) {
                    m_core_frequency_limits[core_idx] = {
                        // Single-element vector because this is the only
                        // tradeoff presented without SST-TF enabled.
                        {cores_in_package.size(), max_frequency}};
                    m_core_lp_frequencies[core_idx] = M_CPU_FREQUENCY_STICKER;
                }
            }
        }
    }

    std::vector<std::pair<unsigned int, double> >
        SSTFrequencyLimitDetector::get_core_frequency_limits(unsigned int core_idx) const
    {
        return m_core_frequency_limits.at(core_idx);
    }

    double SSTFrequencyLimitDetector::get_core_low_priority_frequency(
        unsigned int core_idx) const
    {
        return m_core_lp_frequencies.at(core_idx);
    }

    bool SSTFrequencyLimitDetector::is_frequency_near_limit(
        double frequency, double limit)
    {
        return frequency > (limit - M_CPU_FREQUENCY_STEP);
    }
}
