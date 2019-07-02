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

#ifndef POLICYDATABASE_HPP_INCLUDE
#define POLICYDATABASE_HPP_INCLUDE

#include <string>

struct sqlite3;

namespace geopm
{
    /// Manages a database of best known policies for profiles used with agents.
    /// The database includes the policy strings, the reports associated with
    /// the best known policies, and default policies to apply when a best
    /// run has not yet been recorded.
    class PolicyDatabase final
    {
        public:
            /// Opens a connection to a database, and ensures necessary tables exist
            /// Creates a new database if one does not exist at the given path.
            /// @param database_path Path to the database to open
            PolicyDatabase(const std::string& database_path);

            PolicyDatabase() = delete;
            PolicyDatabase(const PolicyDatabase& other) = delete;
            PolicyDatabase& operator=(const PolicyDatabase& other) = delete;

            /// @brief Destructor for PolicyDatabase
            /// @details Closes the associated database connection.
            ~PolicyDatabase();

            /// @brief Get the best known policy for a given agent/profile pair.
            /// @details Returns the best known policy from the database.  If
            /// no best policy is known, the default policy for the agent is
            /// returned.  An exception is thrown if no default exists, or if
            /// any database errors occur.
            /// @param [in] profile_name Name of the profile to look up
            /// @param [in] agent_name   Name of the agent to look up
            /// @return The recommended policy for the profile to use with the agent.
            std::string GetBestPolicy(const std::string& profile_name,
                                      const std::string& agent_name) const;

            /// @brief Get the best known report for a given agent/profile pair.
            /// @details Returns the best known report from the database.  If
            /// no report exists for the agent/profile pair, an exception is
            /// thrown.  An exception is also thrown if any database errors occur.
            /// @param [in] profile_name Name of the profile to look up
            /// @param [in] agent_name   Name of the agent to look up
            /// @return The best report for the profile when used with the agent.
            std::string GetBestReport(const std::string& profile_name,
                                      const std::string& agent_name) const;

            /// @brief Set the record for the best policy for a profile with an agent.
            /// @details Creates or overwrites the best-known policy for a profile
            /// when used with the given agent.  Also records a report resulting
            /// from this configuration.
            /// @param [in] profile_name Name of the profile whose policy is being set.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] policy Policy string to use with the given agent.
            /// @param [in] report Report resulting from using the agent and policy
            ///                    with the given profile.
            void SetBestPolicy(const std::string& profile_name,
                               const std::string& agent_name,
                               const std::string& policy,
                               const std::string& report);

            /// @brief Set the default policy to use with an agent.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] policy Policy string to use with the given agent.
            void SetDefaultPolicy(const std::string& agent_name,
                                  const std::string& policy);
        private:
            struct sqlite3* m_database;
    };
}

#endif
