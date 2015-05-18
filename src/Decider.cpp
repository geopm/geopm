/*
 * Copyright (c) 2015, Intel Corporation
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

#include <hwloc.h>

#include "Decider.hpp"

namespace geopm
{
    Decider::Decider()
        : m_is_converged(false) {}

    Decider::~Decider() {}

    void Decider::update_policy(const struct geopm_policy_message_s &policy)
    {
        m_phase_policy_msg_map.erase(policy.phase_id);
        m_phase_policy_msg_map.insert(std::pair <long, struct geopm_policy_message_s>(policy.phase_id, policy));
    }

    bool Decider::is_converged(void)
    {
        return m_is_converged;
    }

    LeafDecider::LeafDecider() {}

    LeafDecider::~LeafDecider() {}

    TreeDecider::TreeDecider(int num_children)
        : m_num_children(num_children) {}

    TreeDecider::~TreeDecider() {}

    void TreeDecider::get_policy(Platform const *platform, Policy &policy)
    {
        Phase *cur_phase = platform->cur_phase();
        struct geopm_policy_message_s policy_msg = m_phase_policy_msg_map.find(cur_phase->identifier())->second;
        double target = policy_msg.power_budget / platform->num_domain();
        std::vector <double> target_vec(platform->num_domain());
        std::fill(target_vec.begin(), target_vec.end(), target);
        policy.update(target_vec);
    }


    void TreeDecider::split_policy(const struct geopm_policy_message_s &policy, std::vector <struct geopm_policy_message_s> &split_policy)
    {
        double norm = 1.0 / m_num_children;
        split_policy.resize(m_num_children);
        std::fill(split_policy.begin(), split_policy.end(), policy);
        for (auto policy_it = split_policy.begin(); policy_it != split_policy.end(); ++policy_it) {
            policy_it->power_budget *= norm;
        }
    }

    GoverningDecider::GoverningDecider()
        : m_guard_band(0.05),
          m_package_min_power(13),
          m_board_memory_min_power(7) {}

    GoverningDecider::~GoverningDecider() {}

    void GoverningDecider::get_policy(Platform const *platform, Policy &policy)
    {
        const PlatformTopology topo = platform->topology();
        const PowerModel *package_power_model = platform->power_model(GEOPM_DOMAIN_PACKAGE);
        const PowerModel *board_memory_power_model = platform->power_model(GEOPM_DOMAIN_BOARD_MEMORY);
        const Phase *phase = platform->cur_phase();
        const std::vector <std::string> signal_names({"energy"});
        const double budget = m_phase_policy_msg_map.find(phase->identifier())->second.power_budget;

        std::vector <hwloc_obj_t> package_domain;
        std::vector <hwloc_obj_t> memory_domain;
        std::vector <int> buffer_index;
        std::vector <int> package_domain_index;
        std::vector <int> memory_domain_index;

        topo.domain_by_type(GEOPM_DOMAIN_PACKAGE, package_domain);
        topo.domain_by_type(GEOPM_DOMAIN_BOARD_MEMORY, memory_domain);

        platform->domain_index(GEOPM_DOMAIN_PACKAGE, package_domain_index);
        platform->domain_index(GEOPM_DOMAIN_BOARD_MEMORY, memory_domain_index);

        double package_power = 0.0;
        for (auto domain_it = package_domain.begin(); domain_it != package_domain.end(); ++domain_it) {
            platform->buffer_index(*domain_it, signal_names, buffer_index);
            package_power += package_power_model->power(phase, buffer_index);
        }
        double memory_power = 0.0;
        for (auto domain_it = memory_domain.begin(); domain_it != memory_domain.end(); ++domain_it) {
            platform->buffer_index(*domain_it, signal_names, buffer_index);
            memory_power += board_memory_power_model->power(phase, buffer_index);
        }
        double total_power = package_power + memory_power;

        // Redistribute power if not within guard band of the budget
        if (total_power > budget * (1 + m_guard_band) ||
            total_power < budget * (1 - m_guard_band)) {

            double per_package_target = (budget - memory_power) / package_domain.size();
            double per_memory_target = 0.0;
            if (per_package_target < m_package_min_power) {
                per_package_target = m_package_min_power;
                per_memory_target = (budget - per_package_target * package_domain.size()) / memory_domain.size();
                if (per_memory_target < m_board_memory_min_power) {
                    per_memory_target = m_board_memory_min_power;
                }
            }
            for (auto domain_it = package_domain_index.begin(); domain_it != package_domain_index.end(); ++domain_it) {
                policy.update(*domain_it, per_package_target);
            }
            if (per_memory_target != 0.0) {
                for (auto domain_it = memory_domain_index.begin(); domain_it != memory_domain_index.end(); ++domain_it) {
                    policy.update(*domain_it, per_memory_target);
                }
            }
        }
    }
}
