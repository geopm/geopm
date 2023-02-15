/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dlfcn.h>
#include <stdlib.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <string>

#include "geopm_error.h"
#include "geopm_plugin.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include "config.h"


namespace geopm
{
    void plugin_load(const std::string &plugin_prefix)
    {
        std::string env_plugin_path_str(geopm::get_env("GEOPM_PLUGIN_PATH"));
        std::vector<std::string> plugin_paths {GEOPM_DEFAULT_PLUGIN_PATH};
        std::string so_suffix = ".so." GEOPM_ABI_VERSION;
        std::replace(so_suffix.begin(), so_suffix.end(), ':', '.');

        if (!env_plugin_path_str.empty()) {
            // load paths in reverse order from environment variable list
            auto user_paths = geopm::string_split(env_plugin_path_str, ":");
            std::reverse(user_paths.begin(), user_paths.end());
            plugin_paths.insert(plugin_paths.end(), user_paths.begin(), user_paths.end());
        }
        std::vector<std::string> plugins;
        for (const auto &path : plugin_paths) {
            std::vector<std::string> files = geopm::list_directory_files(path);
            for (const auto &name : files) {
                if (geopm::string_ends_with(name, so_suffix)) {
                    if (geopm::string_begins_with(name, plugin_prefix)) {
                        plugins.push_back(path + "/" + name);
                    }
                }
            }
        }
        for (const auto &plugin : plugins) {
            if (NULL == dlopen(plugin.c_str(), RTLD_NOLOAD)) {
                if (NULL == dlopen(plugin.c_str(), RTLD_LAZY|RTLD_GLOBAL)) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> Failed to dlopen plugin with dlerror(): "
                              << dlerror() << std::endl;
#endif
                }
            }
        }
    }
}
