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
    GEOPM_DOMAIN_INVALID,
    /// @brief All components on a user allocated compute
    ///        node (one per controller)
    GEOPM_DOMAIN_BOARD,
    /// @Brief Single processor package in one socket
    GEOPM_DOMAIN_PACKAGE,
    /// @brief Group of associated hyper-threads
    GEOPM_DOMAIN_CORE,
    /// @brief Linux logical CPU
    GEOPM_DOMAIN_CPU,
    /// @brief Standard off package DIMM (DRAM or NAND)
    GEOPM_DOMAIN_BOARD_MEMORY,
    /// @brief On package memory (MCDRAM)
    GEOPM_DOMAIN_PACKAGE_MEMORY,
    /// @brief Network interface controller on the PCI bus
    GEOPM_DOMAIN_BOARD_NIC,
    /// @brief Network interface controller on the
    ///        processor package
    GEOPM_DOMAIN_PACKAGE_NIC,
    /// @brief Accelerator card on the PCI bus
    GEOPM_DOMAIN_BOARD_ACCELERATOR,
    /// @brief Accelerator unit on the package (e.g
    ///        on-package graphics)
    GEOPM_DOMAIN_PACKAGE_ACCELERATOR,
    GEOPM_NUM_DOMAIN,
};

int geopm_topo_num_domain(int domain_type);

int geopm_topo_domain_idx(int domain_type,
                          int cpu_idx);

int geopm_topo_is_domain_within(int inner_domain,
                                int outer_domain);

int geopm_topo_num_nested_domains(int inner_domain,
                                  int outer_domain,
                                  int outer_idx);

int geopm_topo_nested_domain_idx(int inner_domain,
                                 int outer_domain,
                                 int outer_idx,
                                 int inner_nested_idx);

int geopm_topo_domain_type_to_name(int domain_type,
                                   size_t name_len,
                                   char *name);

int domain_name_to_type(const char *domain_name);

#ifdef __cplusplus
}
#endif
#endif
