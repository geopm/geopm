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

#ifndef MANAGER_HPP_INCLUDE
#define MANAGER_HPP_INCLUDE

#include <string>
#include <vector>

struct sqlite3;

namespace geopm
{
    /// Manages a data store of best known policies for profiles used with agents.
    /// The data store includes records of best known policies and default
    /// policies to apply when a best run has not yet been recorded.
    class Manager final
    {
        public:
            /// Opens a connection to a data store, and ensures necessary tables exist
            /// Creates a new data store if one does not exist at the given path.
            /// @param data_path Path to the data store to open
            Manager(const std::string& data_path);

            Manager() = delete;
            Manager(const Manager& other) = delete;
            Manager& operator=(const Manager& other) = delete;

            /// @brief Destructor for Manager
            /// @details Releases any connections to the associated data store.
            ~Manager();

            /// @brief Get the best known policy for a given agent/profile pair.
            /// @details Returns the best known policy from the data store.  If
            /// no best policy is known, the default policy for the agent is
            /// returned.  An exception is thrown if no default exists, or if
            /// any data store errors occur.
            /// @param [in] profile_name Name of the profile to look up
            /// @param [in] agent_name   Name of the agent to look up
            /// @return The recommended policy for the profile to use with the agent.
            std::vector<double> get_best_policy(const std::string& profile_name,
                                                const std::string& agent_name) const;

            /// @brief Set the record for the best policy for a profile with an agent.
            /// @details Creates or overwrites the best-known policy for a profile
            /// when used with the given agent.
            /// @param [in] profile_name Name of the profile whose policy is being set.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] policy Policy string to use with the given agent.
            void set_best_policy(const std::string& profile_name,
                                 const std::string& agent_name,
                                 const std::vector<double>& policy);

            /// @brief Set the default policy to use with an agent.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] policy Policy string to use with the given agent.
            void set_default_policy(const std::string& agent_name,
                                    const std::vector<double>& policy);
        private:
            struct sqlite3* m_database;
    };
}

#endif
