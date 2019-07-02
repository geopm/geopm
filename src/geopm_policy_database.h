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

#ifndef GEOPM_POLICY_DATABASE_H_INCLUDE
#define GEOPM_POLICY_DATABASE_H_INCLUDE

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct geopm_policy_database;

/*!
 * @brief Open a connection to a database.
 * @details Creates a new database if the given path does not exist.
 * @param [in] database_path Path to the database.
 * @return A pointer to the connection, or NULL if unable to connect. The
 * returned connection is used as an input to other functions in this interface.
 */
struct geopm_policy_database* geopm_policy_database_connect(const char *database_path);

/*!
 * @brief Close the connection to a database.
 * @param [in] database Database to close
 */
void geopm_policy_database_disconnect(struct geopm_policy_database *database);

/*!
 * @brief Get the best known policy for a given agent and profile.
 * @details Gets the agent's default policy if no best policy has
 * been reported.
 * @param [in] database Database to query.
 * @param [in] profile_name Name of the profile to find.
 * @param [in] agent_name Name of the agent to find.
 * @param [in] policy_size Size of @p policy.
 * @param [out] policy Best known or default policy.
 * @return Return zero on success, error code on failure.
 */
int geopm_policy_database_get_best_policy(const struct geopm_policy_database *database,
                                          const char* profile_name, const char* agent_name,
                                          size_t policy_size, char* policy);

/*!
 * @brief Get the report associated with the best known policy for an agent and profile.
 * @param [in] database Database to query.
 * @param [in] profile_name Name of the profile to find.
 * @param [in] agent_name Name of the agent to find.
 * @param [in] report_size Size of @p report.
 * @param [out] report Report associated with the best-known policy.
 * @return Return zero on success, error code on failure.
 */
int geopm_policy_database_get_best_report(const struct geopm_policy_database *database,
                                          const char* profile_name, const char* agent_name,
                                          size_t report_size, char* report);

/*!
 * @brief Set the best known policy for a given agent and profile.
 * @param [in] database Database to update.
 * @param [in] profile_name Name of the profile.
 * @param [in] agent_name Name of the agent.
 * @param [in] policy New policy to apply.
 * @param [in] report Report associated with the new policy.
 * @return Return zero on success, error code on failure.
 */
int geopm_policy_database_set_best_policy(struct geopm_policy_database *database,
                                          const char* profile_name, const char* agent_name,
                                          const char* policy, const char* report);

/*!
 * @brief Set the default policy for a given agent.
 * @param [in] database Database to update.
 * @param [in] agent_name Name of the agent.
 * @param [in] policy Default policy to apply.
 * @return Return zero on success, error code on failure.
 */
int geopm_policy_database_set_default_policy(struct geopm_policy_database *database,
                                             const char* agent_name, const char* policy);

#ifdef __cplusplus
}
#endif
#endif
