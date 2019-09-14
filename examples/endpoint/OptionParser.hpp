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

#include "Exception.hpp"

/// Helper class for parsing command line options
class OptionParser
{
    public:
        OptionParser(const std::string &prog_name);
        /// @brief Add a boolean flag
        void add_option(const std::string &name,
                        char short_form, const std::string &long_form,
                        bool required,
                        bool default_val);
        /// @brief Add an option that takes a string argument
        void add_option(const std::string &name,
                        char short_form, const std::string &long_form,
                        bool required,
                        const std::string &default_val);
        /// @brief Parse and save option values.  Returns whether program
        ///        should exit because -h or -v was passed.
        bool parse(int argc, char *argv[]);
        /// @brief Look up value of an option by name
        template <typename T>
        T get(const std::string &name);
    private:
        void check_add_option(char short_form, const std::string &long_form);
        const int M_MAX_OPTS = 128;
        int m_num_opts;

        struct opt_conf
        {
                // struct option opt;
                std::string name;
                int has_arg;
                //int *flag;
                int val;

                bool required;
        };
        std::map<std::string, opt_conf> m_bool_option_conf;
        std::map<std::string, opt_conf> m_str_option_conf;
        // parsed values
        std::map<std::string, std::string> m_str_vals;
        std::map<std::string, bool> m_bool_vals;
};

template <>
bool OptionParser::get<bool>(const std::string &name)
{
    auto it = m_bool_vals.find(name);
    if (it == m_bool_vals.end()) {
        throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
    }
    return it->second;
}

template <>
std::string OptionParser::get<std::string>(const std::string &name)
{
    auto it = m_str_vals.find(name);
    if (it == m_str_vals.end()) {
        throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
    }
    return it->second;
}

template <typename T>
T OptionParser::get(const std::string &name)
{
    static_assert(false, "OptionParser::get<T>(): Only bool and string value types are defined.");
}


#endif
