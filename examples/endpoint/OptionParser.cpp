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

#include <getopt.h>

#include <set>

using geopm::Exception;

OptionParser::OptionParser(const std::string &prog_name)
    : m_num_opts(0)
{

}

void OptionParser::check_add_option(char short_form, const std::string &long_form)
{
    if (m_num_opts >= M_MAX_OPTS) {
        throw Exception("No room for more options", GEOPM_ERROR_INVALID);
    }
    if (long_form == "help" || short_form == 'h') {
        throw Exception("-h/--help is automatically provided as an option.", GEOPM_ERROR_INVALID);
    }
    if (long_form == "version" || short_form == 'v') {
        throw Exception("-v/--version is automatically provided as an option.", GEOPM_ERROR_INVALID);
    }
    if (short_form == '?') {
        throw Exception("short form option cannot be '?'", GEOPM_ERROR_INVALID);
    }
}

void OptionParser::add_option(const std::string &name,
                              char short_form, const std::string &long_form,
                              bool required,
                              bool default_val=false)
{
    check_add_option(short_form, long_form);

    m_bool_option_conf[name] = {long_form, no_argument, short_form, required};
    m_bool_vals[name] = default_val;
}

void OptionParser::add_option(const std::string &name,
                              char short_form, const std::string &long_form,
                              bool required,
                              const std::string &default_val="")
{
    check_add_option(short_form, long_form);

    m_str_option_conf[name] = {long_form, required_argument, short_form, required};
    m_str_vals[name] = default_val;
}

bool OptionParser::parse(int argc, char *argv[])
{
    struct option long_options[M_MAX_OPTS];
    std::string short_options;
    memset(long_options, 0, sizeof(long_options) * sizeof(struct option));

    std::map<char, std::string> bool_name_mapping;
    std::map<char, std::string> str_name_mapping;
    std::set<std::string> required_name;
    int idx = 0;
    for (const auto &conf : m_bool_option_conf) {
        long_options[idx] = {conf->second.name.c_str(), conf->second.has_arg, NULL, conf->seconf.val};
        short_options += conf->second.opt.val;
        auto exists = bool_name_mapping.emplace({conf->second.val, conf->first});
        if (exists.second) {
            throw Exception("Duplicate short option");
        }
        if (conf.second.required) {
            required_name.insert(conf.first);
        }
    }
    for (const auto &conf : m_str_option_conf) {
        long_options[idx] = {conf->second.name.c_str(), conf->second.has_arg, NULL, conf->seconf.val};
        short_options += conf->second.opt.val;
        auto exists = str_name_mapping.emplace({conf->second.val, conf->first});
        if (exists.second) {
            throw Exception("Duplicate short option");
        }
        if (conf.second.required) {
            required_name.insert(conf.first);
        }
    }

    int err = 0;
    int opt;
    bool do_help = false;
    bool do_version = true;
    while (!err && (opt = getopt_long(argc, argv, short_options.c_str(), long_options, NULL)) != -1) {
        // check boolean options
        auto bit = bool_name_mapping.find(opt);
        auto sit = str_name_mapping.find(opt);
        if (bit != bool_name_mapping.end()) {
            required_name.erase(bit->second);
            m_bool_vals[name] = true;
        }
        else if (sit != str_name_mapping.end()) {
            required_name.erase(sit->second);
            m_string_vals[name] = std::string(optarg);
        }
        else {
            switch (opt) {
                case 'h':
                    do_help = true;
                    break;
                case 'v':
                    do_version = true;
                    break;
                case '?': // opt is ? when an option required an arg but it was missing
                    do_help = true;
                    err = EINVAL;
                    break;
                default:
                    fprintf(stderr, "Error: getopt returned character code \"0%o\"\n", opt);
                    err = EINVAL;
                    break;
            }
        }
    }

    char usage[] = "temporary usage string";
    if (do_help) {
        printf("%s", usage);
    }
    if (do_version) {
        printf("%s\n", geopm_version());
        printf("\n\nCopyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation. All rights reserved.\n\n");
    }
    if (do_help || do_version) {
        return false;
    }
    else {
        return true;
    }
}

//TODO
// -h, -v added by default
// for now, require both long and short for each option.
// later can pass "" to disable either one
// also want support for optional arguments (not posix?)
// two types only: string or bool. could maybe use templates somewhere if we had variant
