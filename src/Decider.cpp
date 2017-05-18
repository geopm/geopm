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

#include <hwloc.h>

#include "Decider.hpp"
#include "config.h"

namespace geopm
{

    Decider::Decider()
        : m_last_power_budget(DBL_MIN)
        , m_upper_bound(DBL_MAX)
        , m_lower_bound(DBL_MIN)
    {

    }

    Decider::Decider(const Decider &other)
        : m_last_power_budget(other.m_last_power_budget)
        , m_upper_bound(other.m_upper_bound)
        , m_lower_bound(other.m_lower_bound)
    {

    }

    Decider::~Decider()
    {

    }

    void Decider::bound(double upper_bound, double lower_bound)
    {
        m_upper_bound = upper_bound;
        m_lower_bound = lower_bound;
    }

    bool Decider::update_policy(const struct geopm_policy_message_s &policy, IPolicy &curr_policy)
    {
        bool result = false;
        if (policy.power_budget != m_last_power_budget) {
            curr_policy.is_converged(GEOPM_REGION_ID_EPOCH, false);
            int num_domain = curr_policy.num_domain();
            // Split the budget up evenly to start.
            double split_budget = policy.power_budget / num_domain;
            std::vector<double> domain_budget(num_domain);
            std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
            curr_policy.update(GEOPM_REGION_ID_EPOCH, domain_budget);
            m_last_power_budget = policy.power_budget;
            result = true;
        }
        return result;
    }
}
