/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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
#include <vector>

#include "Exception.hpp"

namespace geopm
{
    template<class T> class PluginFactory
    {
        public:
            PluginFactory() = default;
            virtual ~PluginFactory() = default;
            PluginFactory(const PluginFactory &other) = delete;
            PluginFactory &operator=(const PluginFactory &other) = delete;
            /// @brief Add a plugin to the factory.
            /// @param [in] plugin_name Name used to request plugins
            ///        of the registered type.
            /// @param [in] make_plugin Function that returns a new
            ///        object of the registered type.
            /// @param [in] dictionary Optional dictionary of static
            ///        information about the registered type.
            void register_plugin(const std::string &plugin_name,
                                 std::function<std::unique_ptr<T>()> make_plugin,
                                 const std::map<std::string, std::string> &dictionary = m_empty_dictionary)
            {
                auto result = m_name_func_map.emplace(plugin_name, make_plugin);
                if (!result.second) {
                    throw Exception("PluginFactory::register_plugin(): name: \"" +
                                    plugin_name + "\" has been previously registered",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                m_dictionary.emplace(plugin_name, dictionary);
                m_plugin_names.push_back(plugin_name);
            }
            /// @brief Create an object of the requested type.  If the
            ///        type was not registered, throws an exception.
            /// @param [in] plugin_name Name used to look up the
            ///        constructor function used to create the object.
            /// @return A unique_ptr to the created object.  The
            ///         caller owns the created object.
            std::unique_ptr<T> make_plugin(const std::string &plugin_name) const
            {
                auto it = m_name_func_map.find(plugin_name);
                if (it == m_name_func_map.end()) {
                    throw Exception("PluginFactory::make_plugin(): name: \"" +
                                    plugin_name + "\" has not been previously registered",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                return it->second();
            }
            /// @brief Returns a list of all valid plugin names
            ///        registered with the factory in the order they
            ///        were registered.
            /// @return List of valid plugin names.
            std::vector<std::string> plugin_names(void) const
            {
                return m_plugin_names;
            }
            /// @brief Returns the dictionary of static metadata about
            ///        a registered type.  If the type was not
            ///        registered, throws an exception.
            /// @param [in] plugin_name Name of the registered type.
            /// @return Dictionary of metadata.
            const std::map<std::string, std::string> &dictionary(const std::string &plugin_name) const
            {
                auto it = m_dictionary.find(plugin_name);
                if (it == m_dictionary.end()) {
                    throw Exception("PluginFactory::dictonary(): Plugin named \"" + plugin_name +
                                    "\" has not been registered with the factory.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                return it->second;
            }
        private:
            std::map<std::string, std::function<std::unique_ptr<T>()> > m_name_func_map;
            std::vector<std::string> m_plugin_names;
            std::map<std::string, const std::map<std::string, std::string> > m_dictionary;
            static const std::map<std::string, std::string> m_empty_dictionary;
    };

    template <class Type>
    const std::map<std::string, std::string> PluginFactory<Type>::m_empty_dictionary = {};

}
#endif
