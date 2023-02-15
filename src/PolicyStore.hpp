/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POLICYSTORE_HPP_INCLUDE
#define POLICYSTORE_HPP_INCLUDE

#include <string>
#include <vector>
#include <memory>

namespace geopm
{
    /// Manages a data store of best known policies for profiles used with agents.
    /// The data store includes records of best known policies and default
    /// policies to apply when a best run has not yet been recorded.
    class PolicyStore
    {
        public:
            PolicyStore() = default;
            virtual ~PolicyStore() = default;

            /// @brief Get the best known policy for a given agent/profile pair.
            /// @details Returns the best known policy from the data store.  If
            /// no best policy is known, the default policy for the agent is
            /// returned.  An exception is thrown if no default exists, or if
            /// any data store errors occur.
            /// @param [in] agent_name   Name of the agent to look up
            /// @param [in] profile_name Name of the profile to look up
            /// @return The recommended policy for the profile to use with the agent.
            virtual std::vector<double> get_best(const std::string &agent_name,
                                                 const std::string &profile_name) const = 0;

            /// @brief Set the record for the best policy for a profile with an agent.
            /// @details Creates or overwrites the best-known policy for a profile
            /// when used with the given agent.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] profile_name Name of the profile whose policy is being set.
            /// @param [in] policy Policy string to use with the given agent.
            virtual void set_best(const std::string &agent_name,
                                  const std::string &profile_name,
                                  const std::vector<double> &policy) = 0;

            /// @brief Set the default policy to use with an agent.
            /// @param [in] agent_name Name of the agent for which this policy applies.
            /// @param [in] policy Policy string to use with the given agent.
            virtual void set_default(const std::string &agent_name,
                                     const std::vector<double> &policy) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<PolicyStore> make_unique(const std::string &data_path);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<PolicyStore> make_shared(const std::string &data_path);
    };
}

#endif
