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
#ifndef GEOPM_PMPI_H_INCLUDE
#define GEOPM_PMPI_H_INCLUDE

/// @brief Swap out COMM_WORLD with our internally modified comm (fortran only)
MPI_Fint geopm_swap_comm_world_f(MPI_Fint comm);
/// @brief Mark entry of a wrapped MPI region
void geopm_mpi_region_enter(uint64_t func_rid);
/// @brief Mark exit of a wrapped MPI region
void geopm_mpi_region_exit(uint64_t func_rid);
/// @brief Create a unique region_id from a MPI function name
uint64_t geopm_mpi_func_rid(const char *func_name);

/* Macro seems to be the best way to deal introducing per function
   static storage with a non-const initializer.  We avoid repeating
   code.  Note this approach is thread safe because of the underlying
   lock in geopm_prof_region(). */
#define GEOPM_PMPI_ENTER_MACRO(FUNC) \
    static unsigned is_once = 1; \
    static uint64_t func_rid = 0; \
    if (is_once) { \
        func_rid = geopm_mpi_func_rid(FUNC); \
        is_once = 0; \
    } \
    geopm_mpi_region_enter(func_rid);

#define GEOPM_PMPI_EXIT_MACRO geopm_mpi_region_exit(func_rid);

#endif //GEOPM_PMPI_H_INCLUDE
