/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
     * @param [in] agent_name Name of the agent to find.
     * @param [in] profile_name Name of the profile to find.
     * @param [in] max_policy_vals Maximum number of values that can fit in @p policy_vals.
     * @param [out] policy_vals Best known or default policy.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_get_best(const char *agent_name, const char *profile_name,
                                   size_t max_policy_vals, double *policy_vals);

    /*!
     * @brief Set the best known policy for a given agent and profile.
     * @param [in] profile_name Name of the profile.
     * @param [in] agent_name Name of the agent.
     * @param [in] num_policy_vals Number of values in @p policy_vals.
     * @param [in] policy_vals New policy to apply.
     * @return Zero on success, error code on failure.
     */
    int geopm_policystore_set_best(const char *agent_name, const char *profile_name,
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
