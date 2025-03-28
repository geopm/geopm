/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dlfcn.h>
#include <stdlib.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <memory>

#include "geopm_error.h"
#include "geopm_plugin.hpp"
#include "geopm_version.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "SecurePath.hpp"



namespace geopm
{
    static bool is_plugin(const std::vector<int> &so_version, const std::string &plugin_prefix, const std::string &name)
    {
        bool result = false;
        if (!geopm::string_begins_with(name, plugin_prefix)) {
            return result;
        }
        size_t suffix_pos = name.rfind(".so.");
        if (suffix_pos == std::string::npos) {
            return result;
        }
        auto suffix = geopm::string_split(name.substr(suffix_pos + 4), ".");
        if (suffix.size() != 3) {
            return result;
        }
        try {
            int major_num = std::stoi(suffix[0]);
            int minor_num = std::stoi(suffix[1]);
            if (major_num == so_version[0] &&
                minor_num <= so_version[1]) {
                result = true;
            }
        }
        catch (const std::invalid_argument &ex) {
        }
        catch (const std::out_of_range &ex) {
        }
        return result;
    }

    void plugin_load(const std::string &plugin_prefix)
    {
        std::string env_plugin_path_str(geopm::get_env("GEOPM_PLUGIN_PATH"));
        std::vector<std::string> plugin_paths {GEOPM_DEFAULT_PLUGIN_PATH};
        if (!env_plugin_path_str.empty()) {
            // load paths in reverse order from environment variable list
            auto user_paths = geopm::string_split(env_plugin_path_str, ":");
            std::reverse(user_paths.begin(), user_paths.end());
            plugin_paths.insert(plugin_paths.end(), user_paths.begin(), user_paths.end());
        }

        const auto so_version = geopm::shared_object_version();
        std::vector<std::shared_ptr<SecurePath>> plugins;
        for (const auto &path : plugin_paths) {
            std::vector<std::string> files = geopm::list_directory_files(path);
            for (const auto &name : files) {
                if (is_plugin(so_version, plugin_prefix, name)) {
                    std::string full_path = path + "/" + name;
                    plugins.push_back(std::make_shared<SecurePath>(full_path));
                }
            }
        }
        for (const auto &plugin : plugins) {
            try {
#ifdef GEOPM_DEBUG
                // gdb will hang if we dlopen the secure file
                std::cerr << "Warning: plugins are not being securely loaded due to --enable-debug compile option\n";
                std::string dl_path = plugin->original_path();
#else
                std::string dl_path = plugin->secure_path();
#endif
                void *dl_handle = dlopen(dl_path.c_str(), RTLD_NOLOAD);
                if (dl_handle == nullptr) {
                    dl_handle = dlopen(dl_path.c_str(), RTLD_NOW);
                    if (dl_handle == nullptr) {
                        std::cerr << "Warning: <geopm> Failed to dlopen plugin ("
                                  << plugin->original_path() << ") with dlerror(): "
                                  << dlerror() << std::endl;
                    }
                }
            }
            catch (const geopm::Exception &ex) {
                std::cerr << "Warning: " << ex.what() << std::endl;
            }
        }
    }

    void plugin_reset(void)
    {

    }
}
