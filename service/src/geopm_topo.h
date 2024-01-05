/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_TOPO_H_INCLUDE
#define GEOPM_TOPO_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_domain_e {
    /*!
     * @brief Reserved to represent an invalid domain
     */
    GEOPM_DOMAIN_INVALID = -1,
    /*!
     * @brief All components on a user allocated compute
     *        node (one per controller)
     */
    GEOPM_DOMAIN_BOARD = 0,
    /*!
     * @brief Single processor package in one socket
     */
    GEOPM_DOMAIN_PACKAGE = 1,
    /*!
     * @brief Group of associated hyper-threads
     */
    GEOPM_DOMAIN_CORE = 2,
    /*!
     * @brief Linux logical CPU
     */
    GEOPM_DOMAIN_CPU = 3,
    /*!
     * @brief Standard off package DIMM (DRAM or NAND)
     */
    GEOPM_DOMAIN_MEMORY = 4,
    /*!
     * @brief On package memory (MCDRAM)
     */
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY = 5,
    /*!
     * @brief Network interface controller on the PCI bus
     */
    GEOPM_DOMAIN_NIC = 6,
    /*!
     * @brief Network interface controller on the
     *        processor package
     */
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC = 7,
    /*!
     * @brief GPU card on the PCI bus
     */
    GEOPM_DOMAIN_GPU = 8,
    /*!
     * @brief GPU unit on the package (e.g
     *        on-package graphics)
     */
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU = 9,
    /*!
     * @brief GPU card chips within a package on
     *        the PCI Bus (e.g Level Zero subdevices)
     */
    GEOPM_DOMAIN_GPU_CHIP = 10,
    /*!
     * @brief Number of valid domains.
     */
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
