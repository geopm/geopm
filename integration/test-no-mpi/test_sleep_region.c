/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <geopm_prof.h>
#include <geopm_hint.h>
#include <geopm_error.h>


int main(int argc, char **argv)
{
    int err = 0;
    uint64_t compute_rid, memory_rid, network_rid;
    geopm_prof_region("compute_region",
                      GEOPM_REGION_HINT_COMPUTE,
                      &compute_rid);
    geopm_prof_region("memory_region",
                      GEOPM_REGION_HINT_MEMORY,
                      &memory_rid);
    geopm_prof_region("network_network",
                      GEOPM_REGION_HINT_NETWORK,
                      &network_rid);
    int num_iter = 10;

    printf("Beginning loop of %d iterations.\n", num_iter);
    fflush(stdout);
    for (int i = 0; !err && i < num_iter; ++i) {
        geopm_prof_epoch();
        geopm_prof_enter(compute_rid);
        sleep(1);
        geopm_prof_exit(compute_rid);
        geopm_prof_enter(memory_rid);
        sleep(1);
        geopm_prof_exit(memory_rid);
        geopm_prof_enter(network_rid);
        sleep(1);
        geopm_prof_exit(network_rid);
        printf("Iteration=%.3d\r", i);
        fflush(stdout);
    }
    printf("Completed loop.                    \n");
    fflush(stdout);
}
