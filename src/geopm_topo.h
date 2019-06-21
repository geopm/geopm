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

#ifndef GEOPM_TOPO_H_INCLUDE
#define GEOPM_TOPO_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_domain_e {
    /// @brief Reserved to represent an invalid domain
    GEOPM_DOMAIN_INVALID = -1,
    /// @brief All components on a user allocated compute
    ///        node (one per controller)
    GEOPM_DOMAIN_BOARD = 0,
    /// @Brief Single processor package in one socket
    GEOPM_DOMAIN_PACKAGE = 1,
    /// @brief Group of associated hyper-threads
    GEOPM_DOMAIN_CORE = 2,
    /// @brief Linux logical CPU
    GEOPM_DOMAIN_CPU = 3,
    /// @brief Standard off package DIMM (DRAM or NAND)
    GEOPM_DOMAIN_BOARD_MEMORY = 4,
    /// @brief On package memory (MCDRAM)
    GEOPM_DOMAIN_PACKAGE_MEMORY = 5,
    /// @brief Network interface controller on the PCI bus
    GEOPM_DOMAIN_BOARD_NIC = 6,
    /// @brief Network interface controller on the
    ///        processor package
    GEOPM_DOMAIN_PACKAGE_NIC = 7,
    /// @brief Accelerator card on the PCI bus
    GEOPM_DOMAIN_BOARD_ACCELERATOR = 8,
    /// @brief Accelerator unit on the package (e.g
    ///        on-package graphics)
    GEOPM_DOMAIN_PACKAGE_ACCELERATOR = 9,
    /// @brief Domain containing mapping of MPI ranks
    ///        to Linux logical CPUs
    GEOPM_DOMAIN_MPI_RANK = 10,
    /// @brief Number of valid domains.
    GEOPM_NUM_DOMAIN = 11,
};

int geopm_topo_num_domain(int domain_type);

int geopm_topo_domain_idx(int domain_type,
                          int cpu_idx);

int geopm_topo_num_domain_nested(int inner_domain,
                                 int outer_domain);

int geopm_topo_domain_nested(int inner_domain,
                             int outer_domain,
                             int outer_idx,
                             size_t num_domain_nested,
                             int *domain_nested);

int geopm_topo_domain_name(int domain_type,
                           size_t domain_name_max,
                           char *domain_name);

int geopm_topo_domain_type(const char *domain_name);

int geopm_topo_create_cache(void);

#ifdef __cplusplus
}
#endif
#endif
