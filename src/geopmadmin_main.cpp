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

#include "config.h"
#include <string>
#include <iostream>
#include <sstream>

#include "Exception.hpp"
#include "OptionParser.hpp"
#include "Admin.hpp"

int main(int argc, char **argv)
{
    int err = 0;
    try {
        geopm::OptionParser parser{"geopmadmin", std::cout, std::cerr, ""};
        parser.add_option("default", 'd', "config-default", false,
                          "print the path of the GEOPM default configuration file");
        parser.add_option("override", 'o', "config-override", false,
                          "print the path of the GEOPM override configuration file");
        parser.add_option("whitelist", 'w', "msr-whitelist", false,
                          "print the minimum msr-safe whitelist required by GEOPM");
        parser.add_option("cpuid", 'c', "cpuid", "-1",
                          "cpuid in hexidecimal for whitelist (default is current platform)");
        parser.add_example_usage("");
        parser.add_example_usage("[--config-default|--config-override|--msr-whitelist] [--cpuid]");
        bool early_exit = parser.parse(argc, argv);
        if (early_exit) {
            return 0;
        }

        auto pos_args = parser.get_positional_args();
        if (pos_args.size() > 0) {
            std::ostringstream err_msg;
            err_msg << "The following positional argument(s) are in error: ";
            for (const std::string &arg : pos_args) {
                err_msg << arg << " ";
            }
            throw geopm::Exception(err_msg.str(), EINVAL, __FILE__, __LINE__);
        }

        bool do_default = parser.is_set("default");
        bool do_override = parser.is_set("override");
        bool do_whitelist = parser.is_set("whitelist");
        int cpuid = std::stoi(parser.get_value("cpuid"), NULL, 16);
        geopm::Admin admin;
        std::cout << admin.main(do_default, do_override, do_whitelist, cpuid);
    }
    catch (...)
    {
        err = geopm::exception_handler(std::current_exception(), false);
    }
    return err;
}
