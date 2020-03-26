/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dlfcn.h>

#include <vector>
#include <algorithm>
#include <iostream>

#include "geopm_error.h"
#include "geopm_plugin.hpp"
#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

#include "config.h"


namespace geopm
{
    void plugin_load(std::string plugin_prefix)
    {
        std::string env_plugin_path_str(geopm::environment().plugin_path());
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
                if (NULL == dlopen(plugin.c_str(), RTLD_LAZY)) {
#ifdef GEOPM_DEBUG
                    std::cerr << "Warning: <geopm> Failed to dlopen plugin with dlerror(): "
                              << dlerror() << std::endl;
#endif
                }
            }
        }
    }
}
