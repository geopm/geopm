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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include <vector>
#include <algorithm>
#include <iostream>

#include "geopm_env.h"
#include "Helper.hpp"

#include "config.h"

static int geopm_name_begins_with(const std::string &str, const std::string &key)
{
    int result = str.find(key) == 0;
    return result;
}

static int geopm_name_ends_with(std::string str, std::string key)
{
    std::reverse(str.begin(), str.end());
    std::reverse(key.begin(), key.end());
    return geopm_name_begins_with(str, key);
}

static const char *GEOPM_AGENT_PLUGIN_PREFIX    = "libgeopmagent_";
static const char *GEOPM_IOGROUP_PLUGIN_PREFIX  = "libgeopmiogroup_";
static const char *GEOPM_COMM_PLUGIN_PREFIX     = "libgeopmcomm_";
static void __attribute__((constructor)) geopmpolicy_load(void)
{
    std::string env_plugin_path_str;
    const char *env_plugin_path = geopm_env_plugin_path();
    DIR *did = NULL;
    std::vector<std::string> plugin_paths {GEOPM_DEFAULT_PLUGIN_PATH};
    std::vector<std::string> plugins;
    std::string so_suffix = ".so." GEOPM_ABI_VERSION;

    if (env_plugin_path) {
        for (auto it = so_suffix.begin(); it != so_suffix.end(); ++it) {
            if (*it == ':') {
                so_suffix.replace(it, it + 1, ".");
            }
        }
        env_plugin_path_str = std::string(env_plugin_path);
        // load paths in reverse order from environment variable list
        auto user_paths = geopm::split_string(env_plugin_path_str, ":");
        std::reverse(user_paths.begin(), user_paths.end());
        plugin_paths.insert(plugin_paths.end(), user_paths.begin(), user_paths.end());
    }
    for (auto &path : plugin_paths) {
        did = opendir(path.c_str());
        if (did) {
            struct dirent *entry;
            while ((entry = readdir(did))) {
                if (geopm_name_ends_with(entry->d_name, so_suffix) ||
                    geopm_name_ends_with(entry->d_name, ".dylib")) {
                    if (geopm_name_begins_with(std::string(entry->d_name), GEOPM_COMM_PLUGIN_PREFIX) ||
                        geopm_name_begins_with(std::string(entry->d_name), GEOPM_IOGROUP_PLUGIN_PREFIX) ||
                        geopm_name_begins_with(std::string(entry->d_name), GEOPM_AGENT_PLUGIN_PREFIX)) {
                        plugins.push_back(path + "/" + std::string(entry->d_name));
                    }
                }
            }
            closedir(did);
        }
    }
    for (auto &plugin : plugins) {
        if (NULL == dlopen(plugin.c_str(), RTLD_NOLOAD)) {
            if (NULL == dlopen(plugin.c_str(), RTLD_LAZY)) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: failed to dlopen plugin " << plugin << std::endl;
#endif
            }
        }
    }
}
