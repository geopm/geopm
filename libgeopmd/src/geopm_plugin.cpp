/*
 * Copyright (c) 2015 - 2024 Intel Corporation
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
#include "geopm_version.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "SecurePath.hpp"

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

    static bool is_plugin(const std::vector<int> &abi_num, const std::string &plugin_prefix, const std::string &name)
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
            if (major_num == abi_num[0] &&
                minor_num <= abi_num[1]) {
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

        const auto abi_num = geopm::version_abi();
        std::vector<std::string> plugins;
        for (const auto &path : plugin_paths) {
            std::vector<std::string> files = geopm::list_directory_files(path);
            for (const auto &name : files) {
                if (is_plugin(abi_num, plugin_prefix, name)) {
                    plugins.push_back(path + "/" + name);
                }
            }
        }
        for (const auto &plugin : plugins) {
            try {
                SecurePath sp (plugin.c_str());
                void *dl_handle = dlopen(sp.secure_path().c_str(), RTLD_NOLOAD);
                if (dl_handle != nullptr) {
                    DLRegistry::dl_registry().add(dl_handle);
                }
                else {
                    dl_handle = dlopen(sp.secure_path().c_str(), RTLD_LAZY|RTLD_GLOBAL);
                    if (dl_handle != nullptr) {
                        DLRegistry::dl_registry().add(dl_handle);
                    }
                    else {
                        std::cerr << "Warning: <geopm> Failed to dlopen plugin ("
                                  << plugin << ") with dlerror(): "
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
        DLRegistry::dl_registry().reset();
    }
}
