/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


/*
#!/bin/bash
# Script bootstraps the example below (simple_pio_example.c)
#
# This example is designed for a systems programmer who wants to build
# geopm::PlatformIO with as few dependencies as possible and make use
# of the geopm_pio_* andn geopm_topo_* interfaces.  This build does
# not have any HPC requirements and should run on any standard Linux
# install that has autotools and a compiler.
#
# The program prints the amount of energy used by package 0 in units
# of joules.
#
# BEGIN SCRIPT

if [ ! -e $HOME/build/geopm/lib/libgeopm.so ]; then
    if [ ! -d geopm ]; then
        git clone https://github.com/geopm/geopm.git
    fi
    cd geopm/
    ./autogen.sh
    ./configure --disable-mpi \
                --disable-openmp \
                --disable-fortran \
                --prefix=$HOME/build/geopm
    make -j10
    make install
    cd ..
fi
gcc -I$HOME/build/geopm/include -L$HOME/build/geopm/lib -lgeopm simple_pio_example.c
LD_LIBRARY_PATH=$HOME/build/geopm/lib:$LD_LIBRARY_PATH ./a.out
# Total energy for package 0: 518.16 (joules)
# END SCRIPT
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

    int err = geopm_pio_read_signal("CPU_ENERGY",
                                    GEOPM_DOMAIN_PACKAGE,
                                    0, &energy0);
    if (!err) {
        sleep(5);
        err = geopm_pio_read_signal("CPU_ENERGY",
                                    GEOPM_DOMAIN_PACKAGE,
                                    0, &energy1);
    }
    if (!err) {
        total_energy = energy1 - energy0;
        printf("Total energy for package 0: %0.2f (joules)\n", total_energy);
    }
    if (err) {
        char error_string[PATH_MAX];
        geopm_error_message(err, error_string, PATH_MAX);
        fprintf(stderr, "Error: %s\n", error_string);
    }
    return err;
}
