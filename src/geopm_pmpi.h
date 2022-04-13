/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_PMPI_H_INCLUDE
#define GEOPM_PMPI_H_INCLUDE

/*!
 * @brief Swap out COMM_WORLD with our internally modified comm (fortran only)
 */
MPI_Fint geopm_swap_comm_world_f(MPI_Fint comm);
/*!
 * @brief Mark entry of a wrapped MPI region
 */
void geopm_mpi_region_enter(uint64_t func_rid);
/*!
 * @brief Mark exit of a wrapped MPI region
 */
void geopm_mpi_region_exit(uint64_t func_rid);
/*!
 * @brief Create a unique region_id from a MPI function name
 */
uint64_t geopm_mpi_func_rid(const char *func_name);

/* Macro seems to be the best way to deal introducing per function
 *  static storage with a non-const initializer.  We avoid repeating
 *  code.  Note this approach is thread safe because of the underlying
 *  lock in geopm_prof_region().
 */
#define GEOPM_PMPI_ENTER_MACRO(FUNC) \
    static unsigned is_once = 1; \
    static uint64_t func_rid = 0; \
    if (is_once || func_rid == 0) { \
        func_rid = geopm_mpi_func_rid(FUNC); \
        is_once = 0; \
    } \
    geopm_mpi_region_enter(func_rid);

#define GEOPM_PMPI_EXIT_MACRO geopm_mpi_region_exit(func_rid);

int geopm_pmpi_init_thread(int *argc, char **argv[], int required, int *provided);
int geopm_pmpi_finalize(void);
MPI_Comm geopm_swap_comm_world(MPI_Comm comm);
#endif /* GEOPM_PMPI_H_INCLUDE */
