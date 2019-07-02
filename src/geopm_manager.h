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

#ifndef geopm_manager_H_INCLUDE
#define geopm_manager_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct geopm_manager_c;

/*!
 * @brief Greate a geopm manager interface.
 * @details Connects to a data store, creating a one if does not exist already.
 * The @p manager is used as an input to other functions in this interface.
 * @param [in] data_path Path to the data store.
 * @param [out] manager Destination of the created manager.
 * @return Zero on success, error code on failure.
 */
int geopm_manager_create(const char *data_path, struct geopm_manager_c **manager);

/*!
 * @brief Destroy a geopm manager interface and release its resources
 * @param [in] manager Geopm manager to destroy.
 * @return Zero on success, error code on failure.
 */
int geopm_manager_destroy(struct geopm_manager_c *manager);

/*!
 * @brief Get the best known policy for a given agent and profile.
 * @details Gets the agent's default policy if no best policy has
 * been reported.
 * @param [in] manager Manager interface to use.
 * @param [in] profile_name Name of the profile to find.
 * @param [in] agent_name Name of the agent to find.
 * @param [in] max_policy_vals Maximum number of values that can fit in @p policy_vals.
 * @param [out] policy_vals Best known or default policy.
 * @return Zero on success, error code on failure.
 */
int geopm_manager_get_best_policy(const struct geopm_manager_c *manager,
                                  const char* profile_name, const char* agent_name,
                                  size_t max_policy_vals, double* policy_vals);

/*!
 * @brief Set the best known policy for a given agent and profile.
 * @param [in] manager Manager interface to use.
 * @param [in] profile_name Name of the profile.
 * @param [in] agent_name Name of the agent.
 * @param [in] num_policy_vals Number of values in @p policy_vals.
 * @param [in] policy_vals New policy to apply.
 * @return Zero on success, error code on failure.
 */
int geopm_manager_set_best_policy(struct geopm_manager_c *manager,
                                  const char* profile_name, const char* agent_name,
                                  size_t num_policy_vals, const double* policy_vals);

/*!
 * @brief Set the default policy for a given agent.
 * @param [in] manager Manager interface to use.
 * @param [in] agent_name Name of the agent.
 * @param [in] num_policy_vals Number of values in @p policy_vals.
 * @param [in] policy_vals Default policy to apply.
 * @return Zero on success, error code on failure.
 */
int geopm_manager_set_default_policy(struct geopm_manager_c *manager,
                                     const char* agent_name,
                                     size_t num_policy_vals, const double* policy_vals);

#ifdef __cplusplus
}
#endif
#endif
