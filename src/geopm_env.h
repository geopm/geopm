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

#ifndef GEOPM_ENV_H_INCLUDE
#define GEOPM_ENV_H_INCLUDE
#ifdef __cplusplus
extern "C"
{
#endif

enum geopm_pmpi_ctl_e {
    GEOPM_PMPI_CTL_NONE,
    GEOPM_PMPI_CTL_PROCESS,
    GEOPM_PMPI_CTL_PTHREAD,
};

const char *geopm_env_policy(void);
const char *geopm_env_shmkey(void);
const char *geopm_env_trace(void);
const char *geopm_env_plugin_path(void);
const char *geopm_env_report(void);
const char *geopm_env_profile(void);
int geopm_env_report_verbosity(void);
int geopm_env_pmpi_ctl(void);
int geopm_env_do_region_barrier(void);
int geopm_env_do_trace(void);
int geopm_env_do_ignore_affinity(void);
int geopm_env_do_profile(void);
int geopm_env_profile_timeout(void);
int geopm_env_debug_attach(void);

#ifdef __cplusplus
}
#endif
#endif
