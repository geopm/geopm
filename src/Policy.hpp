/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <vector>
#include <map>

#include "geopm_message.h"

namespace geopm
{
    class Policy
    {
        public:
            Policy();
            Policy(int num_domain);
            virtual ~Policy();
            void insert_region(uint64_t region_id);
            void update(uint64_t region_id, int domain_idx, double target);
            void update(uint64_t region_id, const std::vector<double> &target);
            void updated_target(uint64_t region_id, std::map<int, double> &target); // map from domain index to updated target value
            void target(uint64_t region_id, std::vector<double> &target);
            void target(uint64_t region_id, int domain, double &target);
            void policy_message(uint64_t region_id, std::vector<geopm_policy_message_s> &message) const;
            void valid_target(uint64_t region_id, std::map<int, double> &target) const;
            /// @brief Set the convergence state.
            /// Called by the decision algorithm when it has determined
            /// whether or not the power policy enforcement has converged
            /// to an acceptance state.
            void is_converged(uint64_t region_id, bool converged_state);
            /// @brief Have we converged for this region.
            /// Set by he decision algorithm when it has determined
            /// that the power policy enforcement has converged to an
            /// acceptance state.
            /// @return true if converged else false.
            bool is_converged(uint64_t region_id) const;

        protected:
            int m_num_domain;
            int m_mode;
            int m_num_sample;
            unsigned long m_flags;
            int m_goal;
            std::map<uint64_t, double> m_budget; // map from region id to total power budget over all domains
            std::map<uint64_t, std::vector<double> > m_target; // map from region id to per domain target vector
            std::map<uint64_t, std::vector<bool> > m_updated; // map from region id to per domain boolean value tracking if the target has been updated
            std::map<uint64_t, bool > m_is_converged;
    };
}

#endif
