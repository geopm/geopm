/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRMFAKEDIRMANAGER_HPP_INCLUDE
#define DRMFAKEDIRMANAGER_HPP_INCLUDE

#include <set>
#include <string>
#include <vector>

class DrmFakeDirManager
{
    public:
        DrmFakeDirManager(std::string base_path_template);
        ~DrmFakeDirManager();
        void create_card(int card_idx);
        void create_card_hwmon(int card_idx, int hwmon_idx);
        void create_tile_in_card(int card_idx, int tile_idx);
        void write_file_in_card_tile(int card_idx, int tile_idx,
                                     const std::string &file_name,
                                     const std::string &contents);

        void write_hwmon_name_and_attribute(int card_index, int hwmon_index,
                                            const std::string &name,
                                            const std::string &attribute,
                                            const std::string &contents);

        std::string get_driver_dir() const;
        void write_local_cpus(int card_index, const std::string &cpu_mask);

    private:
        std::vector<std::string> m_created_dirs;
        std::set<std::string> m_created_files;
        std::string m_base_dir_path;
};
#endif

