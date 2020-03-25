/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef POLICY_HPP_INCLUDE
#define POLICY_HPP_INCLUDE

#include <string>
#include <vector>

namespace geopm
{
    /// Policy encapsulates creating, comparing, and displaying Agent
    /// policies.  A Policy is created with default values for each
    /// field, then later updated with different values as needed.  A
    /// value of NAN indicates that the default should be used.
    ///
    /// Short-term plan: use Policy at "edges" of the runtime and in
    ///     some commandline tools to handle JSON conversions,
    ///     comparisons, filling in NAN as needed.  The main
    ///     Controller loop and Agent interaction remains the same.
    ///     The Agent::validate_policy() method will be used to determine
    ///     default values.
    ///
    /// Long-term plan: extend the Agent interface to use Policy in
    ///     place of vectors of doubles.  Agents will be required to
    ///     provide a function returning a Policy with the default
    ///     values set.  The validate_policy() method will be
    ///     simplified to do validation & clamping only, not replace
    ///     NAN with default.  The TreeComm interface remains the same
    ///     and the Controller will handle converting Policy to vector
    ///     and vice versa.
    ///
    /// To be finalized: new agent API (v2.0) & changes to Controller
    ///     Policy MyAgent::default_policy(void)
    ///     static Policy Agent::default_policy(agent_name) // calls into agent_factory
    ///     vector<Policy> Agent::split_policy(Policy)
    ///     void Agent::adjust_platform(Policy)
    ///     void Agent::validate_policy(Policy) //no longer does NAN replacement
    ///
    /// Example:
    ///     Policy pol = Agent::default_policy("energy_efficient");
    ///     // pol is {1.0e9, 2.1e9, 0.10, 2.1e9}
    ///     pol.update("\"FREQUENCY_MIN\": 1.2e9, \"PERF_MARGIN\", 0.05}");
    ///     // pol is {1.2e9, 2.1e9, 0.05, 2.1e9}
    ///     pol["PERF_MARGIN"] = NAN;
    ///     // pol is {1.2e9, 2.1e9, 0.10, 2.1e9}
    ///     pol.update({NAN, 2, 3, 4});
    ///     // pol is {1.0e9, 2, 3, 4}
    ///     string s = pol.to_json();
    ///     // s is the following or similar:
    ///     // {"FREQ_MIN": 1.0e9, "FREQ_MAX": 2, "PERF_MARGIN": 3, "FREQ_FIXED": 4}
    class Policy
    {
        public:
            struct PolicyField
            {
                std::string name;
                double default_value;
            };
            Policy(const vector<PolicyField> &fields);
            virtual ~Policy() = default;

            /// @todo might want copy/assignment

            /// @brief Return the ordered double policy values as
            ///        vector.  Default values will be used when no
            ///        updated value was set.
            /// @todo Called by Controller for backwards compatibility and
            ///       interactions with TreeComm
            std::vector<double> to_vector(void) const;

            /// @brief Compare two policies for equality.  The values, default or
            ///        overridden, should match exactly in the same order.
            /// @todo Called by Controller walk_down check.
            /// @todo Names should also be checked, but might be expensive.  When
            ///       will we be comparing policies from different agents?
            bool operator==(const Policy &other) const;
            bool operator!=(const Policy &other) const;

            /// @brief Access a policy field by name to read or overwrite.  If
            ///        no new value has been set, or NAN was previously written,
            ///        the default value will be returned.  The name must be
            ///        valid or an exception will be thrown.
            /// @param [in] name Name of the policy field.
            double &operator[](const std::string &name);
            /// @brief Access a policy field by ordered index to read
            ///        or overwrite.  If no new value has been set, or
            ///        NAN was previously written, the default value
            ///        will be returned.  The index must be in bounds or an
            ///        exception will be thrown.
            /// @param [in] index Index of the policy field in order.
            double &operator[](size_t index);

            /// @brief Parse a JSON string and replace any policy
            ///        fields with the values present.  All keys must
            ///        be valid policy names and values must be
            ///        doubles or "NAN" or an exception will be
            ///        thrown.  The string representing "NAN" is not
            ///        case-sensitive.
            /// @todo called by FilePolicy for initial file read of policy
            void update(string json);

            /// @brief Replace all values in the Policy with the
            ///        ordered input.  An input value NAN indicates it
            ///        should be reset to the default.  The vector
            ///        must be the full size of the policy or an
            ///        exception will be thrown.
            /// @todo Called by Controller at treecomm level interactions
            /// @todo could also be implemented with assigment operator. might be confusing.
            void update(vector);

            /// @brief Format the Policy as a JSON string.  All values will be
            ///        added, including defaults.
            /// @todo Called by geopmagent, reporter
            std::string to_json(void) const;
    };
}

#endif
