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
    uint64_t sleep_rid;
    err = geopm_prof_region("tutorial_sleep",
                            GEOPM_REGION_HINT_UNKNOWN,
                            &sleep_rid);
    int num_iter = 10;

    printf("Beginning loop of %d iterations.\n", num_iter);
    fflush(stdout);
    for (int i = 0; !err && i < num_iter; ++i) {
        err = geopm_prof_epoch();
        if (!err) {
            err = geopm_prof_enter(sleep_rid);
        }
        if (!err) {
            sleep(1);
        }
        if (!err) {
            err = geopm_prof_exit(sleep_rid);
        }
        if (!err) {
            printf("Iteration=%.3d\r", i);
            fflush(stdout);
        }
    }
    if (!err) {
        printf("Completed loop.                    \n");
        fflush(stdout);
    }
    else {
        char message[NAME_MAX];
        geopm_error_message(err, message, NAME_MAX);
        fprintf(stderr, "%s\n", message);
    }
    geopm_prof_shutdown();

    return err;
}
