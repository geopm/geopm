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

#include "OptionParser.hpp"

#include <getopt.h>
#include <cstring>

#include <sstream>
#include <algorithm>

#include "geopm_version.h"

#include <iostream>
namespace geopm
{
    static const std::string COPYRIGHT_TEXT = "\nCopyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n\n";

    OptionParser::OptionParser(const std::string &prog_name,
                               std::ostream &std_out,
                               std::ostream &err_out,
                               const std::string &custom_help="")
        : m_prog_name(prog_name)
        , m_std_out(std_out)
        , m_err_out(err_out)
        , m_custom_help(custom_help)
    {
        // automatically support --help and --version
        add_option("help", 'h', "help", false,
                   "print brief summary of the command line usage information, then exit");
        add_option("version", 'v', "version", false,
                   "print version of GEOPM to standard output, then exit");
    }

    bool OptionParser::parse(int argc, char *argv[])
    {
        std::vector<struct option> long_options;
        std::string short_options;


        int idx = 0;
        for (const auto &conf : m_bool_opts) {
            std::string name = conf.first;
            const struct m_opt_parse_s<bool> &opt = conf.second;
            short_options += opt.short_form;
            long_options.push_back({opt.long_form.c_str(), no_argument, NULL, opt.short_form});
            ++idx;
        }
        for (const auto &conf : m_str_opts) {
            std::string name = conf.first;
            const struct m_opt_parse_s<std::string> &opt = conf.second;
            short_options += opt.short_form;
            short_options += ":";
            long_options.push_back({opt.long_form.c_str(), required_argument, NULL, opt.short_form});
        }
        long_options.push_back({NULL, 0, NULL, 0});

        int err = 0;
        bool do_help = false;
        bool do_version = false;
        std::ostringstream msg;
        int opt;
        optind = 1;  // reset to allow multiple parse() calls
        while (!err && (opt = getopt_long(argc, argv, short_options.c_str(), long_options.data(), NULL)) != -1) {
            if (opt == 'h') {
                do_help = true;
            }
            else if (opt == 'v') {
                do_version = true;
            }
            else {
                auto bit = m_bool_short_name.find(opt);
                auto sit = m_str_short_name.find(opt);
                if (bit != m_bool_short_name.end()) {
                    m_bool_opts[bit->second].value = !m_bool_opts.at(bit->second).default_value;  // flip bool value from default if set
                }
                else if (sit != m_str_short_name.end()) {
                    m_str_opts[sit->second].value = std::string(optarg);
                }
                else if (opt == '?') {
                    msg << "Error: invalid option" << std::endl;
                    do_help = true;
                    err = EINVAL;
                }
                else {
                    msg << "Error: getopt returned character code \"" << opt <<  "\"" << std::endl;
                    do_help = true;
                    err = EINVAL;
                }
            }
        }
        m_positional_args.clear();
        while (optind < argc) {
            m_positional_args.emplace_back(argv[optind]);
            ++optind;
        }

        if (do_help && err) {
            m_err_out << format_help();
            m_err_out.flush();
        }
        else if (do_help) {
            m_std_out << format_help();
            m_std_out.flush();
        }
        if (do_version) {
            m_std_out << geopm_version() << "\n";
            m_std_out << COPYRIGHT_TEXT;
            m_std_out.flush();
        }
        if (err) {
            throw Exception(msg.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return do_help || do_version;
    }

    bool OptionParser::is_set(const std::string &name)
    {
        auto it = m_bool_opts.find(name);
        if (it == m_bool_opts.end()) {
            throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
        }
        return it->second.value;
    }

    std::string OptionParser::get_value(const std::string &name)
    {
        auto it = m_str_opts.find(name);
        if (it == m_str_opts.end()) {
            throw geopm::Exception(std::string("Invalid option ") + name, GEOPM_ERROR_INVALID);
        }
        return it->second.value;
    }

    std::vector<std::string> OptionParser::get_positional_args(void)
    {
        return m_positional_args;
    }
    void OptionParser::add_example_usage(const std::string &example)
    {
        m_example_usage.push_back(example);
    }

    std::string OptionParser::format_help(void)
    {
        if (m_custom_help != "") {
            return m_custom_help;
        }
        // put help and version at the end
        m_option_order.erase(std::find(m_option_order.begin(), m_option_order.end(), "help"));
        m_option_order.erase(std::find(m_option_order.begin(), m_option_order.end(), "version"));
        m_option_order.push_back("help");
        m_option_order.push_back("version");

        const int col0 = 2;
        const int col1 = 28;
        const int col2 = 80;
        std::ostringstream tmp;
        tmp << "\n";
        std::string usage_start = "Usage: " + m_prog_name;
        for (auto const &ex_use : m_example_usage) {
            tmp << usage_start << " " << ex_use << "\n";
            usage_start = "       " + m_prog_name;
        }
        tmp << usage_start << " [--help] [--version]\n";
        tmp << "\nMandatory arguments to long options are mandatory for short options too.\n\n";
        for (const auto &name : m_option_order) {
            std::string short_form;
            std::string long_form;
            std::string description;
            auto bit = m_bool_opts.find(name);
            auto sit = m_str_opts.find(name);
            if (bit != m_bool_opts.end()) {
                short_form = bit->second.short_form;
                long_form = bit->second.long_form;
                description = bit->second.description;
            }
            else if (sit != m_str_opts.end()) {
                short_form = sit->second.short_form;
                std::string arg = sit->second.long_form;
                std::transform(arg.begin(), arg.end(), arg.begin(), toupper);
                std::replace(arg.begin(), arg.end(), '-', '_');
                long_form = sit->second.long_form + "=" + arg;
                description = sit->second.description;
            }
            else {
#ifdef GEOPM_DEBUG
                throw Exception("OptionParser::format_help(): invalid option name in ordering",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
            }
            std::string left = std::string(col0, ' ')   + "-" + short_form + ", --" + long_form;
            if (left.size() < col1) {
                left.resize(col1, ' ');
            }
            else {
                left += "\n";
                tmp << left;
                left = std::string(col1, ' ');
            }
            std::string right = "";
            while (description.size() > col2 - col1) {
                auto end = description.find_last_of(" ", col2-col1);
                auto sub = description.substr(0, end);
                tmp << left << sub << "\n";
                left = std::string(col1, ' ');
                description = description.substr(end+1, std::string::npos);
            }
            tmp << left << description << "\n";
        }
        tmp << COPYRIGHT_TEXT;
        return tmp.str();
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
}
