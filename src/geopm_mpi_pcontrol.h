/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef GEOPM_MPI_PCONTROL_H_INCLUDE
#define GEOPM_MPI_PCONTROL_H_INCLUDE

#include <mpi.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_pcontrol_level_e {
    GEOPM_PCONTROL_FLAG_DISABLE = 0,
    GEOPM_PCONTROL_FLAG_ENABLE = 1,
    GEOPM_PCONTROL_FLAG_FLUSH = 2,
    GEOPM_PCONTROL_FLAG_INIT = -1,
    GEOPM_PCONTROL_FLAG_REGISTER = -2,
    GEOPM_PCONTROL_FLAG_REPORT = -3,
    GEOPM_PCONTROL_FLAG_EPOCH_SYNC = -4,
    GEOPM_PCONTROL_FLAG_MAX = 32,
};

int MPI_Pcontrol(int level, ...);

int geopm_mpi_pcontrol(int level,
                       struct geopm_ctl_c *ctl,
                       int in_region_id,
                       const char *region_name,
                       long policy_hint,
                       int *out_region_id);




#ifdef __cplusplus
}
#endif
#endif
