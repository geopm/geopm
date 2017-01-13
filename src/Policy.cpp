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
#include <tuple>
#include <utility>

#include "Exception.hpp"
#include "Policy.hpp"
#include "config.h"

namespace geopm
{
    const double Policy::INVALID_TARGET = -DBL_MAX;

    /// @brief RegionPolicy class encapsulated functionality for policy accounting
    /// at the per-rank level.
    class RegionPolicy
    {
        public:
            RegionPolicy(std::vector<int> &num_domain);
            virtual ~RegionPolicy();
            void update(int ctl_type, int domain_idx, double target);
            void update(int ctl_type, const std::vector<double> &target);
            void target(int ctl_type, std::vector<double> &target);
            void target(int ctl_type, int domain_idx, double &target);
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
            std::vector<int> m_num_domain;
            std::map<int, std::vector<double> > m_target;
            bool m_is_converged;

    };

    Policy::Policy(TelemetryConfig &config)
        : m_policy_flags(0)
    {
        for (int i = 0; i < GEOPM_NUM_CONTROL_DOMAIN; ++i) {
            m_num_domain.push_back(config.num_signal_per_domain(i));
        }
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

    RegionPolicy *Policy::region_policy(uint64_t region_id)
    {
        RegionPolicy *result = NULL;
        auto result_it = m_region_policy.find(region_id);
        if (result_it == m_region_policy.end()) {
            result = new RegionPolicy(m_num_domain);
            m_region_policy.insert(std::pair<uint64_t, RegionPolicy *>(region_id, result));
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

    void Policy::update(uint64_t region_id, int control_type, int domain_idx, double target)
    {
        region_policy(region_id)->update(control_type, domain_idx, target);
    }

    void Policy::update(uint64_t region_id, int control_type, const std::vector <double> &target)
    {
        region_policy(region_id)->update(control_type, target);
    }

    void Policy::mode(int mode)
    {
        m_mode = mode;
    }

    void Policy::policy_flags(unsigned long flags)
    {
        m_policy_flags->flags(flags);
    }


    void Policy::target(uint64_t region_id, int control_type, std::vector <double> &target)
    {
        region_policy(region_id)->target(control_type, target);
    }

    void Policy::target(uint64_t region_id, int control_type, int domain_idx, double &target)
    {
        region_policy(region_id)->target(control_type, domain_idx, target);
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

    RegionPolicy::RegionPolicy(std::vector<int> &num_domain)
        : m_num_domain(num_domain)
        , m_is_converged(false)
    {

    }

    RegionPolicy::~RegionPolicy()
    {

    }

    void RegionPolicy::update(int control_type, int domain_idx, double target)
    {
        if (domain_idx >= 0 && domain_idx < m_num_domain[control_type]) {
            auto target_it = m_target.emplace(std::piecewise_construct,
                                              std::forward_as_tuple(control_type),
                                              std::forward_as_tuple(m_num_domain[control_type], Policy::INVALID_TARGET)).first;
            (*target_it).second[domain_idx] = target;
        }
        else {
            throw Exception("RegionPolicy::update(): domain_index out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::update(int control_type, const std::vector <double> &target)
    {
        if ((int)target.size() == m_num_domain[control_type]) {
            auto empl_ret = m_target.emplace(control_type, target);
            if (!empl_ret.second) {
                (*(empl_ret.first)).second = target;
            }
        }
        else {
            throw Exception("RegionPolicy::update(): target vector not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::target(int control_type, std::vector<double> &target)
    {
        if ((int)target.size() == m_num_domain[control_type]) {
            auto it = m_target.find(control_type);
            if (it != m_target.end()) {
                target = (*it).second;
            }
            else {
                std::fill(target.begin(), target.end(), Policy::INVALID_TARGET);
            }
        }
        else {
            throw Exception("RegionPolicy::target() target vector not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::target(int control_type, int domain_idx, double &target)
    {
        if (domain_idx >= 0 && domain_idx < m_num_domain[control_type]) {
            auto it = m_target.find(control_type);
            if (it != m_target.end()) {
                target = (*it).second[domain_idx];
            }
            else {
                target = Policy::INVALID_TARGET;
            }
        }
        else {
            throw Exception("PolicyRegion::target() domain_idx index out of range", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void RegionPolicy::policy_message(const struct geopm_policy_message_s &parent_msg, std::vector<struct geopm_policy_message_s> &child_msg)
    {
        if ((int)child_msg.size() >= m_num_domain[GEOPM_DOMAIN_CONTROL_POWER]) {
            std::fill(child_msg.begin(), child_msg.begin() + m_num_domain[GEOPM_DOMAIN_CONTROL_POWER], parent_msg);
            auto it = m_target.find(GEOPM_DOMAIN_CONTROL_POWER);
            if (it != m_target.end()) {
                for (int domain_idx = 0; domain_idx != m_num_domain[GEOPM_DOMAIN_CONTROL_POWER]; ++domain_idx) {
                    child_msg[domain_idx].power_budget = (*it).second[GEOPM_DOMAIN_CONTROL_POWER];
                }
            }
            else {
                for (int domain_idx = 0; domain_idx != m_num_domain[GEOPM_DOMAIN_CONTROL_POWER]; ++domain_idx) {
                    child_msg[domain_idx].power_budget = Policy::INVALID_TARGET;
                }
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
