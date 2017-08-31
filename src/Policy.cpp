/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <float.h>

#include "Exception.hpp"
#include "Policy.hpp"
#include "PolicyFlags.hpp"
#include "config.h"

namespace geopm
{
    /// @brief RegionPolicy class encapsulated functionality for policy accounting
    /// at the per-rank level.
    class RegionPolicy
    {
        public:
            RegionPolicy(int num_domain);
            virtual ~RegionPolicy();
            void update(int domain_idx, double target);
            void update(const std::vector<double> &target);
            void target(std::vector<double> &target);
            void target(int domain_idx, double &target);
            void target_updated(std::map<int, double> &target); // map from domain index to updated target value
            void target_valid(std::map<int, double> &target);
            void policy_message(const struct geopm_policy_message_s &parent_msg,
                                std::vector<struct geopm_policy_message_s> &message);
            /// @brief Set the convergence state.
            /// Called by the decision algorithm when it has determined
            /// whether or not the power policy enforcement has converged
            /// to an acceptance state.
            void is_converged(bool converged_state);
            /// @brief Have we converged for this region.
            /// Set by he decision algorithm when it has determined
            /// that the power policy enforcement has converged to an
            /// acceptance state.
            /// @return true if converged else false.
            bool is_converged(void);
        protected:
            const double m_invalid_target;
            const int m_num_domain;
            std::vector<double> m_target;
            std::vector<bool> m_updated;
            bool m_is_converged;

    };

    Policy::Policy(int num_domain)
        : m_policy_flags(NULL)
        , m_num_domain(num_domain)
    {
        //Add the default unmarked region
        (void) region_policy(GEOPM_REGION_ID_EPOCH);
        m_policy_flags = new PolicyFlags(0);
    }

    Policy::~Policy()
    {
        for (auto it = m_region_policy.begin(); it != m_region_policy.end(); ++it) {
            delete (*it).second;
        }
        delete m_policy_flags;
    }

    int Policy::num_domain(void)
    {
        return m_num_domain;
    }

    RegionPolicy *Policy::region_policy(uint64_t region_id)
    {
        RegionPolicy *result = NULL;
        auto result_it = m_region_policy.find(region_id);
        if (result_it == m_region_policy.end()) {
            result = new RegionPolicy(m_num_domain);
            m_region_policy.insert(std::pair<uint64_t, RegionPolicy *>(region_id, result));
            // Give the new region the global power targets
            std::vector<double> budget(m_num_domain);
            target(GEOPM_REGION_ID_EPOCH, budget);
            update(region_id, budget);
        }
        else {
            result = (*result_it).second;
        }
        return result;
    }

    void Policy::region_id(std::vector<uint64_t> &region_id)
    {
        region_id.resize(m_region_policy.size());
        auto vector_it = region_id.begin();
        for (auto map_it = m_region_policy.begin(); map_it != m_region_policy.end(); ++map_it, ++vector_it) {
            *vector_it = (*map_it).first;
        }
    }

    void Policy::update(uint64_t region_id, int domain_idx, double target)
    {
        region_policy(region_id)->update(domain_idx, target);
    }

    void Policy::update(uint64_t region_id, const std::vector <double> &target)
    {
        region_policy(region_id)->update(target);
    }

    void Policy::mode(int mode)
    {
        m_mode = mode;
    }

    void Policy::policy_flags(unsigned long flags)
    {
        m_policy_flags->flags(flags);
    }

    void Policy::target_updated(uint64_t region_id, std::map <int, double> &target)
    {
        region_policy(region_id)->target_updated(target);
    }

    void Policy::target(uint64_t region_id, std::vector <double> &target)
    {
        region_policy(region_id)->target(target);
    }

    void Policy::target(uint64_t region_id, int domain_idx, double &target)
    {
        region_policy(region_id)->target(domain_idx, target);
    }

    int Policy::mode(void) const
    {
        return m_mode;
    }

    int Policy::frequency_mhz(void) const
    {
        return m_policy_flags->frequency_mhz();;
    }

    int Policy::tdp_percent(void) const
    {
        return m_policy_flags->tdp_percent();
    }

    int Policy::affinity(void) const
    {
        return m_policy_flags->affinity();;
    }

    int Policy::goal(void) const
    {
        return m_policy_flags->goal();
    }

    int Policy::num_max_perf(void) const
    {
        return m_policy_flags->num_max_perf();
    }

    void Policy::target_valid(uint64_t region_id, std::map<int, double> &target)
    {
        region_policy(region_id)->target_valid(target);
    }

    void Policy::policy_message(uint64_t region_id,
                                const struct geopm_policy_message_s &parent_msg,
                                std::vector<struct geopm_policy_message_s> &child_msg)
    {
        region_policy(region_id)->policy_message(parent_msg, child_msg);
    }

    void Policy::is_converged(uint64_t region_id, bool converged_state)
    {
        region_policy(region_id)->is_converged(converged_state);
    }

    bool Policy::is_converged(uint64_t region_id)
    {
        return region_policy(region_id)->is_converged();
    }

    RegionPolicy::RegionPolicy(int num_domain)
        : m_invalid_target(-DBL_MAX)
        , m_num_domain(num_domain)
        , m_target(m_num_domain)
        , m_updated(m_num_domain)
        , m_is_converged(false)
    {
        std::fill(m_target.begin(), m_target.end(), m_invalid_target);
        std::fill(m_updated.begin(), m_updated.end(), false);
    }

    RegionPolicy::~RegionPolicy()
    {

    }

    void RegionPolicy::update(int domain_idx, double target)
    {
        if (domain_idx >= 0 && domain_idx < m_num_domain) {
            m_target[domain_idx] = target;
            m_updated[domain_idx] = true;
        }
        else {
            throw Exception("RegionPolicy::update(): domain_index out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::update(const std::vector <double> &target)
    {
        if ((int)target.size() == m_num_domain) {
            m_target = target;
            std::fill(m_updated.begin(), m_updated.end(), true);
        }
        else {
            throw Exception("RegionPolicy::update(): target vector not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::target(std::vector<double> &target)
    {
        if ((int)target.size() == m_num_domain) {
            target = m_target;
            std::fill(m_updated.begin(), m_updated.end(), false);
        }
        else {
            throw Exception("RegionPolicy::target() target vector not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::target(int domain_idx, double &target)
    {
        if (domain_idx >= 0 && domain_idx < m_num_domain) {
            target = m_target[domain_idx];
            m_updated[domain_idx] = false;
        }
        else {
            throw Exception("PolicyRegion::target() domain_idx index out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::target_updated(std::map<int, double> &target)
    {
        target.clear();
        for (int domain_idx = 0; domain_idx < (int)m_target.size(); ++domain_idx) {
            if (m_updated[domain_idx] == true &&
                m_target[domain_idx] != m_invalid_target) {
                target.insert(std::pair<int, double>(domain_idx, m_target[domain_idx]));
                m_updated[domain_idx] = false;
            }
        }
    }

    void RegionPolicy::target_valid(std::map<int, double> &target)
    {
        target.clear();
        for (int domain_idx = 0; domain_idx < m_num_domain; ++domain_idx) {
            if (m_target[domain_idx] != m_invalid_target) {
                target.insert(std::pair <int, double>(domain_idx, m_target[domain_idx]));
            }
        }
    }

    void RegionPolicy::policy_message(const struct geopm_policy_message_s &parent_msg, std::vector<struct geopm_policy_message_s> &child_msg)
    {
        if ((int)child_msg.size() >= m_num_domain) {
            std::fill(child_msg.begin(), child_msg.begin() + m_num_domain, parent_msg);
            std::fill(child_msg.begin() + m_num_domain, child_msg.end(), GEOPM_POLICY_UNKNOWN);
            for (int domain_idx = 0; domain_idx != m_num_domain; ++domain_idx) {
                child_msg[domain_idx].power_budget = m_target[domain_idx];
            }
        }
        else {
            throw Exception("RegionPolicy::policy_message(): message vector improperly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::is_converged(bool converged_state)
    {
        m_is_converged = converged_state;
    }

    bool RegionPolicy::is_converged(void)
    {
        return m_is_converged;
    }
}
