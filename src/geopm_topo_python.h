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

#include "geopm_topo.h"

#ifndef GEOPM_TOPO_PYTHON_H_INCLUDE
#define GEOPM_TOPO_PYTHON_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

enum geopm_domain_python_e {
    DOMAIN_INVALID = GEOPM_DOMAIN_INVALID,
    DOMAIN_BOARD = GEOPM_DOMAIN_BOARD,
    DOMAIN_PACKAGE = GEOPM_DOMAIN_PACKAGE,
    DOMAIN_CORE = GEOPM_DOMAIN_CORE,
    DOMAIN_CPU = GEOPM_DOMAIN_CPU,
    DOMAIN_BOARD_MEMORY = GEOPM_DOMAIN_BOARD_MEMORY,
    DOMAIN_PACKAGE_MEMORY = GEOPM_DOMAIN_PACKAGE_MEMORY,
    DOMAIN_BOARD_NIC = GEOPM_DOMAIN_BOARD_NIC,
    DOMAIN_PACKAGE_NIC = GEOPM_DOMAIN_PACKAGE_NIC,
    DOMAIN_BOARD_ACCELERATOR = GEOPM_DOMAIN_BOARD_ACCELERATOR,
    DOMAIN_PACKAGE_ACCELERATOR = GEOPM_DOMAIN_PACKAGE_ACCELERATOR,
    NUM_DOMAIN = GEOPM_NUM_DOMAIN,
};

int num_domain(int domain_type)
{
    return geopm_topo_num_domain(domain_type);
}

int domain_idx(int domain_type, int cpu_idx)
{
    return geopm_topo_domain_idx(domain_type, cpu_idx);
}

int num_domain_nested(int inner_domain, int outer_domain)
{
    return geopm_topo_num_domain_nested(inner_domain, outer_domain);
}

int domain_nested(int inner_domain, int outer_domain, int outer_idx,
                  size_t num_domain_nested, int *domain_nested)
{
    return geopm_topo_domain_nested(inner_domain, outer_domain, outer_idx,
                                    num_domain_nested, domain_nested);
}

int domain_name(int domain_type, size_t domain_name_max, char *domain_name)
{
    return geopm_topo_domain_name(domain_type, domain_name_max, domain_name);
}

int domain_type(const char *domain_name)
{
    return geopm_topo_domain_type(domain_name);
}

int create_cache(void)
{
    return geopm_topo_create_cache();
}

#ifdef __cplusplus
}
#endif
#endif
