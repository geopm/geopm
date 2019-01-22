/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#ifndef GEOPM_AGENT_H_INCLUDE
#define GEOPM_AGENT_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  @brief Query if system supports an agent type.
 *
 *  @param [in] agent_name Name of agent type.
 *
 *  @return Zero if agent is supported, error code otherwise.
 */
int geopm_agent_supported(const char *agent_name);

/*!
 *  @brief Get number of policy parameters supported by agent.
 *
 *  @param [in] agent_name name of agent type.
 *
 *  @param [out] num_policy Number of policy parameters required by
 *               agent.
 *
 *  @return Zero if agent is supported, error code otherwise.
 */
int geopm_agent_num_policy(const char *agent_name,
                           int *num_policy);

/*!
 *  @brief Get the name of a policy parameter.
 *
 *  @param [in] agent_name Name of agent type.
 *
 *  @param [in] policy_idx Index into policy vector.
 *
 *  @param [in] policy_name_max Number of bytes allocated for the
 *         policy_name string.
 *
 *  @param [out] policy_name The name of the policy controlled by the
 *         indexed entry.  String is unmodified if an error condition
 *         occurs.
 *
 *  @return Zero if agent is supported, policy_idx in range, and
 *          policy name can be stored in output string, error code
 *          otherwise.
 */
int geopm_agent_policy_name(const char *agent_name,
                            int policy_idx,
                            size_t policy_name_max,
                            char *policy_name);

/*!
 *  @brief Create a json file to control agent policy statically.
 *
 *  @param [in] agent_name Name of agent type.
 *
 *  @param [in] policy_array Values for each of the policy parameters
 *         supported by the agent, array length is determined by
 *         the geopm_agent_num_policy() function.
 *
 *  @param [in] json_string_max Number of bytes allocated for
 *         json_string output.
 *
 *  @param [out] json_string Buffer that will be populated with JSON
 *         that can be used to create a policy file.
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_agent_policy_json(const char *agent_name,
                            const double *policy_array,
                            size_t json_string_max,
                            char *json_string);

/*!
 *  @brief The number of sampled paramters provided by agent.
 *
 *  @param [in] agent_name Name of agent type.
 *
 *  @param [out] num_sample The number of values provided by the agent
 *         when the geopm_endpoint_agent_sample() function is called.
 *
 *  @return Zero on success, error code on failure.
 */
int geopm_agent_num_sample(const char *agent_name,
                           int *num_sample);

/*!
 *  @brief The name of the indexed sample value.
 *
 *  @param [in] agent_name Name of agent type.
 *
 *  @param [in] sample_idx Index into sampled parameters.
 *
 *  @param [in] sample_name_max Number of bytes allocated for the
 *         sample_name string.
 *
 *  @param [out] sample_name The name of the sample parameter provided
 *         by the indexed entry when the geopm_endpoint_agent_sample()
 *         function is called.  String is unmodified if an error
 *         condition occurs.
 *
 *  @return Zero if agent is supported, error code otherwise.
 */
int geopm_agent_sample_name(const char *agent_name,
                            int sample_idx,
                            size_t sample_name_max,
                            char *sample_name);


/*!
 *  @brief The number of available agents.
 *
 *  @param [out] num_agent The number of agents currently available.
 *
 *  @return Zero if no error occured.  Otherwise the error code will be returned.
 */
int geopm_agent_num_avail(int *num_agent);

/*!
 *  @brief The name of a specific agent.
 *
 *  @param [in] agent_idx The index of the agent in question.
 *
 *  @param [in] agent_name_max Number of bytes allocated for the
 *              agent_name string.
 *
 *  @param [out] agent_name The name of the agent parameter provided
 *         by the indexed entry when the geopm_endpoint_num_agent()
 *         function is called.  String is unmodified if an error
 *         condition occurs.
 *
 *  @return Zero if agent_name is large enough to hold the name,
 *          error code otherwise.
 */
int geopm_agent_name(int agent_idx,
                     size_t agent_name_max,
                     char *agent_name);

#ifdef __cplusplus
}
#endif
#endif
