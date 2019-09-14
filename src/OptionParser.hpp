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

#ifndef OPTIONPARSER_HPP_INCLUDE
#define OPTIONPARSER_HPP_INCLUDE

#include <string>
#include <map>
#include <ostream>
#include <vector>

#include "Exception.hpp"

namespace geopm
{
    class OptionParser
    {
        public:
            OptionParser(const std::string &prog_name,
                         std::ostream &std_out,
                         std::ostream &err_out,
                         const std::string &custom_help);
            /// @brief Add an option.  Boolean types do not require an
            ///        argument; all other types do.
            template <typename T>
            void add_option(const std::string &name,
                            char short_form, const std::string &long_form,
                            T default_val,
                            const std::string &description);
            /// @brief Parse and save option values.  Returns whether program
            ///        should exit because -h or -v was passed.
            bool parse(int argc, char *argv[]);
            /// @brief Look up value of an option by name
            template <typename T>
            T get(const std::string &name);
            /// @brief Returns all positional arguments after parsed options.
            std::vector<std::string> get_positional_args(void);
            /// @brief Add an example to the usage output. The program
            ///        name and indentation should be omitted.
            void add_example_usage(const std::string &example);
            /// @brief Returns the usage string containing description
            ///        of all options.
            std::string format_help(void);
        private:
            void check_add_option(const std::string &name, char short_form,
                                  const std::string &long_form);
            std::string m_prog_name;
            std::ostream &m_std_out;
            std::ostream &m_err_out;
            std::string m_custom_help;
            std::vector<std::string> m_example_usage;
            std::vector<std::string> m_option_order;

            template <typename T>
            struct m_opt_parse_s
            {
                char short_form;
                std::string long_form;
                T value;
                T default_value;
                std::string description;
            };
            std::map<std::string, struct m_opt_parse_s<bool> > m_bool_opts;
            std::map<std::string, struct m_opt_parse_s<std::string> > m_str_opts;
            std::map<char, std::string> m_bool_short_name;
            std::map<char, std::string> m_str_short_name;


            std::vector<std::string> m_positional_args;
    };

    template <>
    bool OptionParser::get<bool>(const std::string &name)
    {
        auto it = m_bool_opts.find(name);
        if (it == m_bool_opts.end()) {
            throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
        }
        return it->second.value;
    }

    template <>
    std::string OptionParser::get<std::string>(const std::string &name)
    {
        auto it = m_str_opts.find(name);
        if (it == m_str_opts.end()) {
            throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
        }
        return it->second.value;
    }

    void OptionParser::check_add_option(const std::string &name, char short_form,
                                        const std::string &long_form)
    {
        if (short_form == '?') {
            throw Exception("OptionParser::check_add_option(): cannot have ? as a short option",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        for (auto const &opt : m_bool_opts) {
            if (short_form == opt.second.short_form) {
                throw Exception(std::string("OptionParser::check_add_option(): short form ") + short_form +
                                " already assigned to an option.", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);

            }
            if (long_form == opt.second.long_form) {
                throw Exception(std::string("OptionParser::check_add_option(): long form ") + long_form +
                                " already assigned to an option.", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            }
        }
        for (auto const &opt : m_str_opts) {
            if (short_form == opt.second.short_form) {
                throw Exception(std::string("OptionParser::check_add_option(): short form ") + short_form +
                                " already assigned to an option.", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            }
            if (long_form == opt.second.long_form) {
                throw Exception(std::string("OptionParser::check_add_option(): long form ") + long_form +
                                " already assigned to an option.", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            }
        }
    }

    template <>
    void OptionParser::add_option<const std::string &>(const std::string &name,
                                                       char short_form, const std::string &long_form,
                                                       const std::string &default_val,
                                                       const std::string &description)
    {
        check_add_option(name, short_form, long_form);
        m_str_opts[name] = {short_form, long_form, default_val, default_val, description};
        m_str_short_name[short_form] = name;
        m_option_order.push_back(name);
    }

    // force string literals to use string specialization
    template <>
    void OptionParser::add_option<const char *>(const std::string &name,
                                                char short_form, const std::string &long_form,
                                                const char *default_val,
                                                const std::string &description)
    {
        add_option<const std::string &>(name, short_form, long_form, default_val, description);
    }

    template <>
    void OptionParser::add_option<bool>(const std::string &name,
                                        char short_form, const std::string &long_form,
                                        bool default_val,
                                        const std::string &description)
    {
        check_add_option(name, short_form, long_form);
        m_bool_opts[name] = {short_form, long_form, default_val, default_val, description};
        m_bool_short_name[short_form] = name;
        m_option_order.push_back(name);
    }

    // disable other types
    template <typename T>
    T OptionParser::get(const std::string &name)
    {
        static_assert(false, "OptionParser::get<T>(): value type not supported.");
    }

    template <typename T>
    void OptionParser::add_option(const std::string &name,
                                  char short_form, const std::string &long_form,
                                  T default_val,
                                  const std::string &description)
    {
        static_assert(false, "OptionParser::add_option<T>(): option type not supported.");
    }
}

#endif
