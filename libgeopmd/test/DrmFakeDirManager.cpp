/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmFakeDirManager.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "geopm/Helper.hpp"

DrmFakeDirManager::DrmFakeDirManager(std::string base_path_template)
{
    if (mkdtemp(&base_path_template[0]) == nullptr) {
        throw std::runtime_error("Could not create a temporary directory at " +
                                 base_path_template);
    }
    m_base_dir_path = std::string(base_path_template);
    m_created_dirs.push_back(m_base_dir_path);

    auto meaningless_dir_path = m_base_dir_path + "/something_else";

    if (mkdir(meaningless_dir_path.c_str(), 0755) == -1) {
        rmdir(m_base_dir_path.c_str());
        throw std::runtime_error("Could not create directory at " + meaningless_dir_path);
    }
    m_created_dirs.push_back(meaningless_dir_path);
}

DrmFakeDirManager::~DrmFakeDirManager()
{
    for (const auto &file_path : m_created_files) {
        unlink(file_path.c_str());
    }
    // Clean up directories we created in reverse order so
    // any attempted directory removals are on empty directories
    for (auto it = m_created_dirs.rbegin(); it != m_created_dirs.rend(); ++it) {
        rmdir(it->c_str());
    }
}

void DrmFakeDirManager::create_card(int card_idx)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_idx;
    auto new_path = oss.str();
    if (mkdir(new_path.c_str(), 0755) == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not create directory at " + new_path);
    }
    m_created_dirs.push_back(new_path);
    oss << "/gt";
    new_path = oss.str();
    if (mkdir(new_path.c_str(), 0755) == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not create directory at " + new_path);
    }
    m_created_dirs.push_back(new_path);
}

void DrmFakeDirManager::create_card_hwmon(int card_idx, int hwmon_idx)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_idx << "/device/hwmon";
    auto new_path = oss.str();
    errno = 0;
    if (mkdir(new_path.c_str(), 0755) == -1 && errno != EEXIST) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not create directory at " + new_path);
    }
    if (errno != EEXIST) {
        m_created_dirs.push_back(new_path);
    }
    oss << "/hwmon" << hwmon_idx;
    new_path = oss.str();
    if (mkdir(new_path.c_str(), 0755) == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not create directory at " + new_path);
    }
    m_created_dirs.push_back(new_path);
}

void DrmFakeDirManager::create_tile_in_card(int card_idx, int tile_idx)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_idx << "/gt/gt" << tile_idx;
    auto tile_path = oss.str();
    if (mkdir(tile_path.c_str(), 0755) == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not create directory at " + tile_path);
    }
    m_created_dirs.push_back(tile_path);
}

void DrmFakeDirManager::write_file_in_card_tile(int card_idx, int tile_idx,
                                                const std::string &file_name,
                                                const std::string &contents)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_idx << "/gt/gt" << tile_idx << "/" << file_name;
    auto file_path = oss.str();
    geopm::write_file(file_path, contents);
    m_created_files.insert(file_path);
}

void DrmFakeDirManager::write_hwmon_name_and_attribute(int card_index, int hwmon_index, const std::string &name,
                                                       const std::string &attribute, const std::string &contents)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_index << "/device/hwmon/hwmon" << hwmon_index;
    auto hwmon_dir_path = oss.str();
    geopm::write_file(hwmon_dir_path + "/name", name);
    m_created_files.insert(hwmon_dir_path + "/name");
    geopm::write_file(hwmon_dir_path + "/" + attribute, contents);
    m_created_files.insert(hwmon_dir_path + "/" + attribute);
}

std::string DrmFakeDirManager::get_driver_dir() const
{
    return m_base_dir_path;
}

void DrmFakeDirManager::write_local_cpus(int card_index, const std::string &cpu_mask)
{
    std::ostringstream oss;
    oss << m_base_dir_path << "/card" << card_index << "/device/local_cpus";
    geopm::write_file(oss.str(), cpu_mask);
}
