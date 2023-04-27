/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstdint>
#include <string.h>
#include <mpi.h>
#include <limits.h>
#include <unistd.h>
#include <algorithm>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm_error.h"
#include "geopm_sched.h"
#include "geopm/PlatformTopo.hpp"
#include "GEOPMBenchConfig.hpp"
#include "ModelApplication.hpp"
#include "ModelParse.hpp"
#include "config.h"

int main(int argc, char **argv)
{
    int err = 0;
    int rank;
    int verbosity = 0;
    int do_markup_init = 1;
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
"                 spin: Executes a spin loop for big-o seconds.\n"
"\n"
"                 stream: Executes stream \"triadd\" on a vector with\n"
"                 length proportional to big-o.\n"
"\n"
"                 dgemm: Dense matrix-matrix multiply with floating\n"
"                 point operations proportional to big-o.\n"
"\n"
"                 all2all: All processes send buffers to all other\n"
"                 processes.  The time of this operation is\n"
"                 proportional to big-o.\n"
"\n"
"                 Example configuration json string with imbalance and\n"
"                 progress:\n"
"\n"
"                 {\"loop-count\": 10,\n"
"                  \"region\": [\"sleep\", \"stream-progress\", \"dgemm-imbalance\", \"stream\", \"all2all\"],\n"
"                  \"big-o\": [1.0, 1.0, 1.0, 1.0, 1.0],\n"
"                  \"hostname\": [\"compute-node-3\", \"compute-node-15\"],\n"
"                  \"imbalance\": [0.05, 0.15]}\n"
"\n"
"                 If \"-imbalance\" is appended to any region name in\n"
"                 the configuration file and the \"hostname\" and\n"
"                 \"imbalance\" fields are provided then those\n"
"                 regions will have an injected delay on the hosts\n"
"                 listed.  In the above example a 5%% delay on\n"
"                 \"my-compute-node-3\" and a 15%% delay on\n"
"                 \"my-compute-node-15\" are injected when executing\n"
"                 the dgemm region.\n"
"\n"
"                 If \"-progress\" is appended to any region name in the\n"
"                 configuration, then progress for the region will be\n"
"                 reported through the geopm_tprof_* API.\n"
"\n"
"\n";

    const int ERROR_HELP = -4096;
    const geopm::GEOPMBenchConfig &config = geopm::geopmbench_config();
    if (config.is_mpi_enabled()) {
        err = MPI_Init(&argc, &argv);
        if (!err) {
            err = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        }
    }

    if (!err && argc > 1) {
        if (strncmp(argv[1], "--help", strlen("--help")) == 0 ||
            strncmp(argv[1], "-h", strlen("-h")) == 0) {
            if (!rank) {
                printf(usage, argv[0], argv[0]);
            }
            err = ERROR_HELP;
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

    uint64_t loop_count = 0;
    std::vector<std::string> region_sequence;
    std::vector<double> big_o_sequence;

    int cpu_idx = geopm_sched_get_cpu();
    int package_idx = geopm::platform_topo().domain_idx(GEOPM_DOMAIN_PACKAGE, cpu_idx);

    if (!err) {
        if (config_path) {
            geopm::model_parse_config(config_path, loop_count, region_sequence, big_o_sequence);
        }
        else {
            // Default values if no configuration is specified
            loop_count = 10;
            region_sequence = {"sleep", "stream", "dgemm", "stream", "all2all"};
            big_o_sequence = {1.0, 1.0, 1.0, 1.0, 1.0};
        }

        if (region_sequence.size() > 0 &&
            std::all_of(region_sequence.cbegin(), region_sequence.cend(),
                        [](const std::string &region) {
                            return (region.find("-unmarked") != std::string::npos);
                        })) {
            do_markup_init = 0;
        }
    }

    if (!err && do_markup_init == 1) {
        err = geopm_prof_region("model-init", GEOPM_REGION_HINT_UNKNOWN, &init_rid);
    }
    if (!err && do_markup_init == 1) {
        err = geopm_prof_enter(init_rid);
    }
    if (!err) {
        // Do application initialization
        geopm::ModelApplication app(loop_count, region_sequence, big_o_sequence, verbosity, rank);
        if (do_markup_init == 1) {
            err = geopm_prof_exit(init_rid);
        }
        if (!err) {
            sleep(5);
            // Run application
            app.run();
        }
    }

    if (err == ERROR_HELP) {
        err = 0;
    }

    if (err) {
        char err_msg[PATH_MAX] = {};
        geopm_error_message(err, err_msg, PATH_MAX);
        std::cerr << "ERROR: " << argv[0] << ": " << err_msg << std::endl;
    }

    if (config.is_mpi_enabled()) {
        int err_fin = MPI_Finalize();
        err = err ? err : err_fin;
    }

    return err;
}
