/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <cmath>

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <vector>

#include "geopm_version.h"
#include "geopm_error.h"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"

#include "config.h"

using geopm::PlatformIO;
using geopm::PlatformTopo;

int parse_domain_type(const std::string &dom);
static int main_imp(int argc, char **argv);

int main(int argc, char **argv)
{
    int err = 0;
    try {
        err = main_imp(argc, argv);
    }
    catch (const geopm::Exception &ex) {
        std::cerr << "Error: geopmwrite: " << ex.what() << "\n\n";
        err = ex.err_value();
    }
    return err;
}

static int main_imp(int argc, char **argv)
{
    const char *usage = "\nUsage:\n"
                        "       geopmwrite CONTROL_NAME DOMAIN_TYPE DOMAIN_INDEX VALUE\n"
                        "       geopmwrite [--info [CONTROL_NAME]]\n"
                        "       geopmwrite [--help] [--version] [--cache] [--info-all] [--domain]\n"
                        "\n"
                        "  CONTROL_NAME:  name of the control\n"
                        "  DOMAIN_TYPE:  name of the domain for which the control should be written\n"
                        "  DOMAIN_INDEX: index of the domain, starting from 0\n"
                        "  VALUE:        setting to adjust control to\n"
                        "\n"
                        "  -d, --domain                     print domains detected\n"
                        "  -i, --info                       print longer description of a control\n"
                        "  -I, --info-all                   print longer description of all controls\n"
                        "  -c, --cache                      create geopm topo cache if it does not exist\n"
                        "  -h, --help                       print brief summary of the command line\n"
                        "                                   usage information, then exit\n"
                        "  -v, --version                    print version of GEOPM to standard output,\n"
                        "                                   then exit\n"
                        "\n"
                        "Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.\n"
                        "\n";

    static struct option long_options[] = {
        {"domain", no_argument, NULL, 'd'},
        {"info", no_argument, NULL, 'i'},
        {"info-all", no_argument, NULL, 'I'},
        {"cache", no_argument, NULL, 'c'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    int err = 0;
    bool is_domain = false;
    bool is_info = false;
    bool is_all_info = false;
    while (!err && (opt = getopt_long(argc, argv, "diIchv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                is_domain = true;
                break;
            case 'i':
                is_info = true;
                break;
            case 'I':
                is_all_info = true;
                break;
            case 'c':
                geopm::PlatformTopo::create_cache();
                return 0;
            case 'h':
                printf("%s", usage);
                return 0;
            case 'v':
                printf("%s\n", geopm_version());
                printf("\n\nCopyright (c) 2015 - 2024, Intel Corporation. All rights reserved.\n\n");
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

    PlatformIO &platform_io = geopm::platform_io();
    const PlatformTopo &platform_topo = geopm::platform_topo();
    if (is_domain) {
        // print all domains
        for (int dom = GEOPM_DOMAIN_BOARD; dom < GEOPM_NUM_DOMAIN; ++dom) {
            std::cout << std::setw(28) << std::left
                      << PlatformTopo::domain_type_to_name(dom)
                      << platform_topo.num_domain(dom) << std::endl;
        }
    }
    else if (is_info) {
        try {
            // print description for one control
            if (pos_args.size() > 0) {
                std::cout << pos_args[0] << ":" << std::endl
                          << platform_io.control_description(pos_args[0]) << std::endl;
            }
            else {
                std::cerr << "Error: no control requested." << std::endl;
                err = EINVAL;
            }
        }
        catch (const geopm::Exception &ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            err = EINVAL;
        }
    }
    else if (is_all_info) {
        // print all controls with description
        auto controls = platform_io.control_names();
        for (const auto &con : controls) {
            std::cout << con << ":" << std::endl
                      << platform_io.control_description(con) << std::endl;
        }
    }
    else {
        if (pos_args.size() == 0) {
            // print all controls
            auto controls = platform_io.control_names();
            for (const auto &con : controls) {
                std::cout << con << std::endl;
            }
        }
        else if (pos_args.size() >= 4) {
            // write control
            std::string control_name = pos_args[0];
            int domain_idx = -1;
            double write_value = NAN;
            try {
                domain_idx = std::stoi(pos_args[2]);
            }
            catch (const std::invalid_argument&) {
                std::cerr << "Error: invalid domain index.\n" << std::endl;
                err = EINVAL;
            }
            if (!err) {
                try {
                    write_value = std::stod(pos_args[3]);
                }
                catch (const std::invalid_argument&) {
                    std::cerr << "Error: invalid write value.\n" << std::endl;
                    err = EINVAL;
                }
            }
            if (!err) {
                try {
                    int domain_type = PlatformTopo::domain_name_to_type(pos_args[1]);
                    platform_io.write_control(control_name, domain_type, domain_idx, write_value);
                }
                catch (const geopm::Exception &ex) {
                    std::cerr << "Error: cannot write control: " << ex.what() << std::endl;
                    err = EINVAL;
                }
            }
        }
        else {
            std::cerr << "Error: domain type, domain index, and value are required to write control.\n" << std::endl;
            err = EINVAL;
        }
    }

    return err;
}
