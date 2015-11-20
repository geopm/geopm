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

#include "geopm_message.h"
#include "Decider.hpp"

namespace geopm
{
    static const std::string gov_decider_desc = "governing";

    Decider::Decider()
        : m_is_converged(false) {}

    Decider::~Decider() {}

    void Decider::update_policy(const struct geopm_policy_message_s &policy, Region* curr_region)
    {
        curr_region->last_policy(m_region_policy_msg_map.find(policy.region_id)->second);
        m_region_policy_msg_map.erase(policy.region_id);
        m_region_policy_msg_map.insert(std::pair <long, struct geopm_policy_message_s>(policy.region_id, policy));
    }

    bool Decider::is_converged(void)
    {
        return true;//m_is_converged;
    }

    LeafDecider::LeafDecider() {}

    LeafDecider::~LeafDecider() {}

    TreeDecider::TreeDecider() {}

    TreeDecider::~TreeDecider() {}

    void TreeDecider::get_policy(Platform const *platform, Policy &policy)
    {
        Region *cur_region = platform->cur_region();
        struct geopm_policy_message_s policy_msg = m_region_policy_msg_map.find(cur_region->identifier())->second;
        double target = policy_msg.power_budget / platform->num_domain();
        std::vector <double> target_vec(platform->num_domain());
        std::fill(target_vec.begin(), target_vec.end(), target);
        policy.update(target_vec);
    }


    void TreeDecider::split_policy(const struct geopm_policy_message_s &policy, Region *region)
    {
        int num_child = region->child_sample()->size();
        double norm = 1.0 / num_child;
        std::vector<geopm_policy_message_s> *spolicy = region->split_policy();
        spolicy->resize(num_child);
        std::fill(spolicy->begin(), spolicy->end(), policy);
        for (auto policy_it = spolicy->begin(); policy_it != spolicy->end(); ++policy_it) {
            policy_it->power_budget *= norm;
        }
    }
}
