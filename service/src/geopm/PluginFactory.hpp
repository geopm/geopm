/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            ///
            /// @param [in] plugin_name Name used to request plugins
            ///        of the registered type.
            ///
            /// @param [in] make_plugin Function that returns a new
            ///        object of the registered type.
            ///
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
            ///
            /// @param [in] plugin_name Name used to look up the
            ///        constructor function used to create the object.
            ///
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
            ///
            /// @return List of valid plugin names.
            std::vector<std::string> plugin_names(void) const
            {
                return m_plugin_names;
            }
            /// @brief Returns the dictionary of static metadata about
            ///        a registered type.  If the type was not
            ///        registered, throws an exception.
            ///
            /// @param [in] plugin_name Name of the registered type.
            ///
            /// @return Dictionary of metadata.
            const std::map<std::string, std::string> &dictionary(const std::string &plugin_name) const
            {
                auto it = m_dictionary.find(plugin_name);
                if (it == m_dictionary.end()) {
                    throw Exception("PluginFactory::dictionary(): Plugin named \"" + plugin_name +
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
