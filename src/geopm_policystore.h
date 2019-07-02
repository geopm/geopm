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

#ifndef GEOPM_POLICYSTORE_H_INCLUDE
#define GEOPM_POLICYSTORE_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*!
     * @brief Create a geopm policy store interface.
     * @details Connects to a data store, creating a one if does not exist already.
     * @param [in] data_path Path to the data store.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_connect(const char *data_path);

    /*!
     * @brief Destroy a geopm policy store interface and release its resources
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_disconnect();

    /*!
     * @brief Get the best known policy for a given agent and profile.
     * @details Gets the agent's default policy if no best policy has
     * been reported.
     * @param [in] profile_name Name of the profile to find.
     * @param [in] agent_name Name of the agent to find.
     * @param [in] max_policy_vals Maximum number of values that can fit in @p policy_vals.
     * @param [out] policy_vals Best known or default policy.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_get_best(const char *profile_name, const char *agent_name,
                                   size_t max_policy_vals, double *policy_vals);

    /*!
     * @brief Set the best known policy for a given agent and profile.
     * @param [in] profile_name Name of the profile.
     * @param [in] agent_name Name of the agent.
     * @param [in] num_policy_vals Number of values in @p policy_vals.
     * @param [in] policy_vals New policy to apply.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_set_best(const char *profile_name, const char *agent_name,
                                   size_t num_policy_vals, const double *policy_vals);

    /*!
     * @brief Set the default policy for a given agent.
     * @param [in] agent_name Name of the agent.
     * @param [in] num_policy_vals Number of values in @p policy_vals.
     * @param [in] policy_vals Default policy to apply.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_set_default(const char *agent_name, size_t num_policy_vals,
                                      const double *policy_vals);

#ifdef __cplusplus
}
#endif
#endif
