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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include "geopm_version.h"
#include "geopm_error.h"
#include "geopm_env.h"
#include "geopm_hash.h"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"

#include "config.h"

using geopm::IPlatformIO;
using geopm::IPlatformTopo;

int parse_domain_type(const std::string &dom);

int main(int argc, char **argv)
{
    const char *usage = "\nUsage:\n"
                        "       geopmread SIGNAL_NAME DOMAIN_TYPE DOMAIN_INDEX\n"
                        "       geopmread [--domain [SIGNAL_NAME]]\n"
                        "       geopmread [--info [SIGNAL_NAME]]\n"
                        "       geopmread [--help] [--version]\n"
                        "\n"
                        "  SIGNAL_NAME:  name of the signal\n"
                        "  DOMAIN_TYPE:  name of the domain for which the signal should be read\n"
                        "  DOMAIN_INDEX: index of the domain, starting from 0\n"
                        "\n"
                        "  -d, --domain                     print domain of a signal\n"
                        "  -i, --info                       print longer description of a signal\n"
                        "  -h, --help                       print brief summary of the command line\n"
                        "                                   usage information, then exit\n"
                        "  -v, --version                    print version of GEOPM to standard output,\n"
                        "                                   then exit\n"
                        "\n"
                        "Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation. All rights reserved.\n"
                        "\n";

    static struct option long_options[] = {
        {"domain", no_argument, NULL, 'd'},
        {"info", no_argument, NULL, 'i'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    int err = 0;
    bool is_domain = false;
    bool is_info = false;
    while (!err && (opt = getopt_long(argc, argv, "dihv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                is_domain = true;
                break;
            case 'i':
                is_info = true;
                break;
            case 'h':
                printf("%s", usage);
                return 0;
            case 'v':
                printf("%s\n", geopm_version());
                printf("\n\nCopyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation. All rights reserved.\n\n");
                return 0;
            case '?': // opt is ? when an option required an arg but it was missing
                fprintf(stderr, usage, argv[0]);
                err = EINVAL;
                break;
            default:
                fprintf(stderr, "Error: getopt returned character code \"0%o\"\n", opt);
                err = EINVAL;
                break;
        }
    }

    if (is_domain && is_info) {
        std::cerr << "Error: info about domain not implemented." << std::endl;
        return EINVAL;
    }

    std::vector<std::string> pos_args;
    while (optind < argc) {
        pos_args.emplace_back(argv[optind++]);
    }

    IPlatformIO &platform_io = geopm::platform_io();
    IPlatformTopo &platform_topo = geopm::platform_topo();
    if (is_domain) {
        if (pos_args.size() == 0) {
            // print all domains
            for (int dom = IPlatformTopo::M_DOMAIN_BOARD; dom < IPlatformTopo::M_NUM_DOMAIN; ++dom) {
                std::cout << std::setw(24) << std::left
                          << IPlatformTopo::domain_type_to_name(dom)
                          << platform_topo.num_domain(dom) << std::endl;
            }
        }
        else {
            // print domain for one signal
            try {
                int domain_type = platform_io.signal_domain_type(pos_args[0]);
                std::cout << IPlatformTopo::domain_type_to_name(domain_type) << std::endl;
            }
            catch (const geopm::Exception &ex) {
                std::cerr << "Error: unable to determine signal type: " << ex.what() << std::endl;
                err = EINVAL;
            }
        }
    }
    else if (is_info) {
        try {
            if (pos_args.size() == 0) {
                // print all signals with description
                auto signals = platform_io.signal_names();
                for (const auto &sig : signals) {
                    /// @todo nicer formatting
                    std::cout << sig << ": " << platform_io.signal_description(sig) << std::endl;
                }
            }
            else {
                // print description for one signal
                std::cout << pos_args[0] << ": " << platform_io.signal_description(pos_args[0]) << std::endl;
            }
        }
        catch (const geopm::Exception &ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            err = EINVAL;
        }
    }
    else {
        if (pos_args.size() == 0) {
            // print all signals
            auto signals = platform_io.signal_names();
            for (const auto &sig : signals) {
                std::cout << sig << std::endl;
            }
        }
        else if (pos_args.size() >= 3) {
            // read signal
            std::string signal_name = pos_args[0];
            int domain_idx = -1;
            try {
                domain_idx = std::stoi(pos_args[2]);
            }
            catch (const std::invalid_argument &) {
                std::cerr << "Error: invalid domain index.\n" << std::endl;
                err = EINVAL;
            }
            if (!err) {
                try {
                    int domain_type = IPlatformTopo::domain_name_to_type(pos_args[1]);
                    int idx = platform_io.push_signal(signal_name, domain_type, domain_idx);
                    // read_batch multiple times for derivative-based signals;
                    // should not affect other signals
                    for (int ii = 0; ii < 16; ++ii) {
                        platform_io.read_batch();
                        platform_io.sample(idx);
                        usleep(5000);
                    }
                    double result = platform_io.sample(idx);
                    if (signal_name.find("#") == std::string::npos) {
                        std::cout << result << std::endl;
                    }
                    else {
                        // signals with # in the name are hex fields
                        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(16)
                                  << geopm_signal_to_field(result) << std::endl;
                    }
                }
                catch (const geopm::Exception &ex) {
                    std::cerr << "Error: cannot read signal: " << ex.what() << std::endl;
                    err = EINVAL;
                }
            }
        }
        else {
            std::cerr << "Error: domain type and domain index are required to read signal.\n" << std::endl;
            err = EINVAL;
        }
    }
    return err;
}
