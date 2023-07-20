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
#include <dlfcn.h>

#include "geopm_error.h"
#include "geopm_plugin.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include "config.h"


namespace geopm
{
    class DLRegistry
    {
        public:
            static DLRegistry &dl_registry(void);
            DLRegistry(const DLRegistry &other) = delete;
            DLRegistry &operator=(const DLRegistry &other) = delete;
            void add(void *handle);
            virtual ~DLRegistry();
            void reset(void);
        private:
            DLRegistry() = default;
            std::vector<void *> m_handles;
    };

    DLRegistry &DLRegistry::dl_registry(void)
    {
        static DLRegistry instance;
        return instance;
    }

    DLRegistry::~DLRegistry()
    {
        reset();
    }

    void DLRegistry::add(void *handle)
    {
        m_handles.push_back(handle);
    }

    void DLRegistry::reset(void)
    {
        for (auto &it : m_handles) {
            if (dlclose(it) != 0) {
                std::cerr << "Warning: <geopm> Failed to dlclose(3) an active shared object handle\n";
            }
        }
        m_handles.clear();
    }

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
            void *dl_handle = dlopen(plugin.c_str(), RTLD_NOLOAD);
            if (dl_handle != nullptr) {
                DLRegistry::dl_registry().add(dl_handle);
            }
            else {
                dl_handle = dlopen(plugin.c_str(), RTLD_LAZY|RTLD_GLOBAL);
                if (dl_handle != nullptr) {
                    DLRegistry::dl_registry().add(dl_handle);
                }
#ifdef GEOPM_DEBUG
                else {

                    std::cerr << "Warning: <geopm> Failed to dlopen plugin with dlerror(): "
                              << dlerror() << std::endl;
                }
#endif
            }
        }
    }

    void plugin_reset(void)
    {
        DLRegistry::dl_registry().reset();
    }
}
