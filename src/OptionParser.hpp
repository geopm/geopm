/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OPTIONPARSER_HPP_INCLUDE
#define OPTIONPARSER_HPP_INCLUDE

#include <string>
#include <map>
#include <ostream>
#include <vector>

#include "geopm/Exception.hpp"

namespace geopm
{
    class OptionParser
    {
        public:
            OptionParser(const std::string &prog_name,
                         std::ostream &std_out,
                         std::ostream &err_out);
            OptionParser(const std::string &prog_name,
                         std::ostream &std_out,
                         std::ostream &err_out,
                         const std::string &custom_help);
            /// @brief Add an option with a string argument.
            void add_option(const std::string &name,
                            char short_form,
                            const std::string &long_form,
                            const std::string &default_val,
                            const std::string &description);
            void add_option(const std::string &name,
                            char short_form,
                            const std::string &long_form,
                            const char *default_val,
                            const std::string &description);
            /// @brief Add a boolean flag option.
            void add_option(const std::string &name,
                            char short_form,
                            const std::string &long_form,
                            bool default_val,
                            const std::string &description);
            /// @brief Parse and save option values.  Returns whether program
            ///        should exit because -h or -v was passed.
            bool parse(int argc, const char * const argv[]);
            /// @brief Check the value of a boolean option
            bool is_set(const std::string &name);
            /// @brief Look up value of a string option by name
            std::string get_value(const std::string &name);
            /// @brief Returns all positional arguments after parsed options.
            std::vector<std::string> get_positional_args(void);
            /// @brief Add an example to the usage output. The program
            ///        name and indentation should be omitted.
            void add_example_usage(const std::string &example);
            /// @brief Returns the usage string containing description
            ///        of all options.
            std::string format_help(void);
        private:
            void check_add_option(const std::string &name,
                                  char short_form,
                                  const std::string &long_form);
            void format_option(std::ostream &tmp,
                               const std::string &short_form,
                               const std::string &long_form,
                               std::string description);

            static const std::string M_COPYRIGHT_TEXT;

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
}

#endif
