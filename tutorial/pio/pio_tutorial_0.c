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

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include "geopm_topo.h"
#include "geopm_pio.h"
#include "geopm_error.h"

int main(int argc, char **argv)
{
    double energy0 = 0.0;
    double energy1 = 0.0;
    double total_energy = 0.0;

    int err = geopm_pio_read_signal("ENERGY_PACKAGE",
                                    GEOPM_DOMAIN_PACKAGE,
                                    0, &energy0);
    if (!err) {
        sleep(5);
        err = geopm_pio_read_signal("ENERGY_PACKAGE",
                                    GEOPM_DOMAIN_PACKAGE,
                                    0, &energy1);
    }
    if (!err) {
        total_energy = energy1 - energy0;
        printf("Total energy for package 0: %0.2f (joules)\n", total_energy);
    }
    if (err) {
        char error_string[NAME_MAX];
        geopm_error_message(err, error_string, NAME_MAX);
        fprintf(stderr, "Error: %s\n", error_string);
    }
    return err;
}
