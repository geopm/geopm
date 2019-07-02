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
#include "PolicyStore.hpp"

#include "geopm_error.h"
#include "geopm_policystore.h"

extern "C"
{
    int geopm_policystore_connect(const char *data_path)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policystore_disconnect()
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policystore_get_best(const char *profile_name, const char *agent_name,
                                   size_t max_policy_vals, double *policy_vals)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policystore_set_best(const char *profile_name, const char *agent_name,
                                   size_t num_policy_vals, const double *policy_vals)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }

    int geopm_policystore_set_default(const char *agent_name, size_t num_policy_vals,
                                      const double *policy_vals)
    {
        return GEOPM_ERROR_NOT_IMPLEMENTED;
    }
}
