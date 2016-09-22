/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

#include "geopm.h"
#include "imbalancer.h"
#include "Exception.hpp"
#include "ModelApplication.hpp"

#ifndef NAME_MAX
#define NAME_MAX 512
#endif

int main(int argc, char **argv)
{
    int err = 0;
    int rank;
    int verbosity = 0;
    uint64_t init_rid;
    char *config_path = NULL;
    const char *usage = "\n"
"%s -h | --help\n"
"    Print this help message.\n"
"\n"
"%s [--verbose] [config_file]\n"
"\n"
"    --verbose: Print output from rank zero as every region executes.\n"
"\n"
"    config_file: Path to json file containing loop count and sequence\n"
"                 of regions in each loop.\n"
"\n"
"                 Example configuration json string:\n"
"\n"
"                 {\"loop-count\": 10,\n"
"                  \"region\": [\"sleep\", \"stream\", \"dgemm\", \"stream\", \"all2all\"],\n"
"                  \"big-o\": [1.0, 1.0, 1.0, 1.0, 1.0]}\n"
"\n"
"                 The \"loop-count\" value is an integer that sets the\n"
"                 number of loops executed.  Each time through the loop\n"
"                 the regions listed in the \"region\" array are\n"
"                 executed.  The \"big-o\" array gives double precision\n"
"                 values for each region.  Region names can be one of\n"
"                 the following options:\n"
"\n"
"                 sleep: Executes clock_nanosleep() for big-o seconds.\n"
"\n"
"                 stream: Executes stream \"triadd\" on a vector with\n"
"                 length proportional to big-o.\n"
"\n"
"                 dgemm: Dense matrix vector multiply with floating\n"
"                 point operations proportional to big-o.\n"
"\n"
"                 all2all: All processes send buffers to all other\n"
"                 processes.  The time of this operation is\n"
"                 proportional to big-o.\n"
"\n"
"\n"
"        If \"-imbalance\" is appended to any region name in the\n"
"        configuration file then the \"IMBALANCER_CONFIG\" environment\n"
"        variable will be used to delay processes on selected hosts for\n"
"        the region.  The \"IMBALANCER_CONFIG\" environment variable\n"
"        should be set to point to a text file with two entries on each\n"
"        line.  The first entry is the hostname to be delayed and the\n"
"        second column is the fractional delay added to the region.\n"
"        Example imbalancer config file:\n"
"\n"
"            my-compute-node-3 0.05\n"
"            my-compute-node-15 0.15\n"
"\n"
"        This would inforce a 5% delay on my-compute-node-3 and a 15%\n"
"        delay on my-compute-node-15.\n"
"\n"
"\n";

    err = MPI_Init(&argc, &argv);
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }

    if (!err && argc > 1) {
        if (strncmp(argv[1], "--help", strlen("--help")) == 0 ||
            strncmp(argv[1], "-h", strlen("-h")) == 0) {
            if (!rank) {
                printf(usage, argv[0], argv[0]);
            }
            return 0;
        }
        int offset = 1;
        if (strncmp(argv[1], "--verbose", strlen("--verbose")) == 0) {
            if (!rank) {
                verbosity = 1;
            }
            ++offset;
        }
        if (argc > offset) {
            config_path = argv[offset];
        }
    }

    if (!err) {
        err = geopm_prof_region("model-init", GEOPM_POLICY_HINT_UNKNOWN, &init_rid);
    }
    if (!err) {
        err = geopm_prof_enter(init_rid);
    }
    if (!err) {
        // Do application initialization
        uint64_t loop_count = 0;
        std::vector<std::string> region_sequence;
        std::vector<double> big_o_sequence;
        std::vector<std::string> hostname;
        std::vector<double> imbalance;
        if (config_path) {
            geopm::model_parse_config(config_path, loop_count,
                                      region_sequence, big_o_sequence,
                                      hostname, imbalance);
            if (region_sequence.size() != big_o_sequence.size() ||
                hostname.size() != imbalance.size()) {
                throw geopm::Exception("Array length mismatch in configuration file " + std::string(config_path),
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (hostname.size()) {
                char hostname_tmp[NAME_MAX];
                hostname_tmp[NAME_MAX - 1] = '\0';
                if (gethostname(hostname_tmp, NAME_MAX - 1)) {
                    throw geopm::Exception("gethostname():", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                std::string this_hostname(hostname_tmp);
                auto hostname_it = hostname.begin();
                for (auto imbalance_it = imbalance.begin(); imbalance_it != imbalance.end(); ++imbalance_it, ++hostname_it) {
                    if (this_hostname == *hostname_it) {
                        imbalancer_frac(*imbalance_it);
                    }
                }
            }
        }
        else {
            // Default values if no configuration is specified
            loop_count = 10;
            region_sequence = {"sleep", "stream", "dgemm", "stream", "all2all"};
            big_o_sequence = {1.0, 1.0, 1.0, 1.0, 1.0};
        }
        geopm::ModelApplication app(loop_count, region_sequence, big_o_sequence, verbosity, rank);
        err = geopm_prof_exit(init_rid);
        if (!err) {
            // Run application
            app.run();
        }
    }
    if (!err) {
        err = MPI_Finalize();
    }
    if (err) {
        char err_msg[NAME_MAX];
        geopm_error_message(err, err_msg, NAME_MAX);
        std::cerr << "ERROR: " << argv[0] << ": " << err_msg << std::endl;
    }
    return err;
}
