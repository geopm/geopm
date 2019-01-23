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

#ifndef GEOPM_ENDPOINT_H_INCLUDE
#define GEOPM_ENDPOINT_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct geopm_endpoint_c;

/*!
 *  @brief Create an endpoint object for other API functions.
 *
 *  @param [in] endpoint_name Shared memory key substring used to create
 *         an endpoint that an agent can attach to.
 *
 *  @param [out] endpoint Opaque pointer to geopm_endpoint_c object,
 *          or NULL upon failure.
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_create(const char *endpoint_name,
                          struct geopm_endpoint_c **endpoint);

/*!
 *  @brief Release resources associated with endpoint.
 *
 *  Additionally will send a signal to the agent that the manager
 *  is detaching from the policy and will no longer send updates.
 *
 *  @param [in] endpoint Object to be destroyed which was previously
 *         created by call to geopm_endpoint_create().
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_destroy(struct geopm_endpoint_c *endpoint);

/*!
 *  @brief Create shmem regions within the endpoint for policy/sample
 *         handling.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create().
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_shmem_create(struct geopm_endpoint_c *endpoint);

/*!
 *  @brief Destroy shmem regions within the endpoint.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create().
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_shmem_destroy(struct geopm_endpoint_c *endpoint);

/*!
 *  @brief Attach an endpoint to existing shmem regions.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create().
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_shmem_attach(struct geopm_endpoint_c *endpoint);

/*!
 *  @brief Check if an agent has attached.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create().
 *
 *  @param [in] agent_name_max Number of bytes allocated for the
 *         agent_name string.
 *
 *  @param [out] agent_name Name of agent if one has attached to the
 *         endpoint.  If no agent has attached string is unaltered.
 *
 *  @return Zero if endpoint has an agent attached and agent name can
 *          be stored in provided buffer, error code otherwise.  If no
 *          agent has attached, error code GEOPM_ERROR_NO_AGENT is
 *          returned.
 *
 */
int geopm_endpoint_agent(struct geopm_endpoint_c *endpoint,
                         size_t agent_name_max,
                         char *agent_name);

/*!
 *  @brief Get the number of nodes managed by the agent.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create() that has reported an attached
 *         agent.
 *
 *  @param [out] num_node Number of compute nodes controled by
 *         attached agent.
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_endpoint_num_node(struct geopm_endpoint_c *endpoint,
                            int *num_node);

/*!
 *  @brief Get the hostname of the indexed compute node.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create() that has reported an attached
 *         agent.
 *
 *  @param [in] node_idx the index from zero to the value returned by
 *         geopm_agent_num_node() - 1.
 *
 *  @param [in] node_name_max Number of bytes allocated for the
 *         node_name string.
 *
 *  @param [out] node_name The compute node hostname.
 *
 *  @return Zero on success, error code on failure
 */
int geopm_endpoint_node_name(struct geopm_endpoint_c *endpoint,
                             int node_idx,
                             size_t node_name_max,
                             char *node_name);

/*!
 *  @brief Set the policy values for the agent to follow.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create() that has reported an attached
 *         agent.
 *
 *  @param [in] policy_array Array of length returned by
 *         geopm_agent_num_policy() specifing the value of each policy
 *         parameter.
 *
 *  @return Zero on success, error code on failure
 */
int geopm_endpoint_agent_policy(struct geopm_endpoint_c *endpoint,
                                const double *policy_array);


/*!
 *  @brief Get a sample from the agent and amount of time that has
 *         passed since the agent last provided an update.
 *
 *  @param [in] endpoint Object created by call to
 *         geopm_endpoint_create() that has reported an attached agent
 *         that provides samples (i.e. value given by
 *         geopm_agent_num_sample() is greater than zero).
 *
 *  @param [out] sample_array Array of sampled values provide by the
 *         agent.
 *
 *  @param [out] sample_age_sec A single value of time elapsed in
 *         units of seconds between agent's last update to the sample
 *         and the time of call to geomp_endpoint_agent_sample().
 *
 *  @return Zero on success, error code on failure
 */
int geopm_endpoint_agent_sample(struct geopm_endpoint_c *endpoint,
                                double *sample_array,
                                double *sample_age_sec);

#ifdef __cplusplus
}
#endif
#endif
