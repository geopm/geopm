/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

#include "config.h"

static const char *GEOPM_AGENT_PLUGIN_PREFIX    = "libgeopmagent_";
static const char *GEOPM_IOGROUP_PLUGIN_PREFIX  = "libgeopmiogroup_";
static const char *GEOPM_COMM_PLUGIN_PREFIX     = "libgeopmcomm_";
static int env_plugin_path(std::string &plugin_path)
{
    int err = 0;
    try {
        plugin_path = geopm::environment().plugin_path();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}

static void __attribute__((constructor)) geopmpolicy_load(void)
{
    int err = 0;
    std::string env_plugin_path_str;
    err = env_plugin_path(env_plugin_path_str);
    std::vector<std::string> plugin_paths {GEOPM_DEFAULT_PLUGIN_PATH};
    std::string so_suffix = ".so." GEOPM_ABI_VERSION;

    if (!err && !env_plugin_path_str.empty()) {
        for (auto it = so_suffix.begin(); it != so_suffix.end(); ++it) {
            if (*it == ':') {
                so_suffix.replace(it, it + 1, ".");
            }
        }
        // load paths in reverse order from environment variable list
        auto user_paths = geopm::string_split(env_plugin_path_str, ":");
        std::reverse(user_paths.begin(), user_paths.end());
        plugin_paths.insert(plugin_paths.end(), user_paths.begin(), user_paths.end());
    }
    std::vector<std::string> plugins;
    for (const auto &path : plugin_paths) {
        std::vector<std::string> files = geopm::list_directory_files(path);
        for (const auto &name : files) {
            if (geopm::string_ends_with(name, so_suffix) ||
                geopm::string_ends_with(name, ".dylib")) {
                if (geopm::string_begins_with(name, GEOPM_COMM_PLUGIN_PREFIX) ||
                    geopm::string_begins_with(name, GEOPM_IOGROUP_PLUGIN_PREFIX) ||
                    geopm::string_begins_with(name, GEOPM_AGENT_PLUGIN_PREFIX)) {
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
