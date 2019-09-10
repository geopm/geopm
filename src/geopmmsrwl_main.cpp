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

#include <iostream>
#include <string>
#include "geopm_version.h"
#include "MSRIOGroup.hpp"
#include "Exception.hpp"

int main(int argc, char **argv)
{
    int err = 0;
    int cpuid = -1;
    bool is_help = false;
    bool is_version = false;

    if (argc == 2) {
        std::string argv1(argv[1]);
        if (argv1 == "--help") {
            is_help = true;
        }
        else if (argv1 == "--version") {
            is_version = true;
        }
        else {
            cpuid = std::stoi(argv1, 0, 16);
        }
    }
    if (argc > 2) {
        err = -1;
        is_help = true;
    }
    if (is_help) {
            std::cerr << "Usage: " << argv[0] << " [cpuid]\n"
                      << "       Print the msr-safe whitelist for host CPU or\n"
                      << "       cpuid if specified in hex on the command line.\n\n";
    }
    else if (is_version) {
        std::cerr << geopm_version() << "\n\n"
                  << "Copyright (c) 2015, 2016, 2017, 2018, 2019, "
                  << "Intel Corporation. All rights reserved.\n\n";
    }
    else {
        try {
            if (cpuid != -1) {
                std::cout << geopm::MSRIOGroup::msr_whitelist(cpuid);
            }
            else {
                geopm::MSRIOGroup msriog;
                std::cout <<  msriog.msr_whitelist();
            }
        }
        catch (geopm::Exception ex) {
            std::cerr << "Error: " << ex.what() << "\n\n";
            err = -1;
        }
    }
    return err;
}
