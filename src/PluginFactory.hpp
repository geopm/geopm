/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef PLUGINFACTORY_HPP_INCLUDE
#define PLUGINFACTORY_HPP_INCLUDE

#include <map>
#include <memory>
#include <string>
#include <functional>

#include "Exception.hpp"


namespace geopm
{
    template<class T> class PluginFactory
    {
        public:
            PluginFactory() = default;
            virtual ~PluginFactory() = default;
            void register_plugin(const std::string &plugin_name,
                                 std::function<std::unique_ptr<T>()> make_plugin)
            {
                auto result = m_name_func_map.emplace(plugin_name, make_plugin);
                if (!result.second) {
                    throw Exception("PluginFactory::register_plugin(): name: \"" +
                                    plugin_name + "\" has been previously registered",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            std::unique_ptr<T> make_plugin(const std::string &plugin_name)
            {
                auto it = m_name_func_map.find(plugin_name);
                if (it == m_name_func_map.end()) {
                    throw Exception("PluginFactory::make_plugin(): name: \"" +
                                    plugin_name + "\" has not been previously registered",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                return it->second();
            }
            static PluginFactory<T> &plugin_factory(void)
            {
                static PluginFactory<T> instance;
                return instance;
            }
        private:
            PluginFactory(const PluginFactory &other) = delete;
            PluginFactory &operator=(const PluginFactory &other) = delete;

            std::map<std::string, std::function<std::unique_ptr<T>()> > m_name_func_map;
    };

}
#endif
