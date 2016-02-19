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

#include <hwloc.h>

#include "geopm_message.h"
#include "geopm_plugin.h"
#include "BalancingDecider.hpp"
#include "Exception.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::Decider *decider = new geopm::BalancingDecider;
            geopm_factory_register(factory, decider);
        }
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception());
    }

    return err;
}

namespace geopm
{
    BalancingDecider::BalancingDecider()
        : m_name("balancing")
        , m_convergence_target(0.05)
        , m_min_num_converged(5)
        , m_num_converged(0)
        , m_last_power_budget(DBL_MIN)
    {

    }

    BalancingDecider::~BalancingDecider()
    {

    }

    Decider *BalancingDecider::clone(void) const
    {
        return (Decider*)(new BalancingDecider(*this));
    }

    bool BalancingDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& BalancingDecider::name(void) const
    {
        return m_name;
    }

    bool BalancingDecider::update_policy(const struct geopm_policy_message_s &policy_msg, Policy &curr_policy)
    {
        bool result = false;
        if (policy_msg.power_budget != m_last_power_budget) {
            curr_policy.is_converged(GEOPM_REGION_ID_OUTER, false);
            int num_domain = curr_policy.num_domain();
            if (m_last_power_budget != DBL_MIN) {
                // Split the budget up by the same ratio we split the old budget.
                for (int i = 0; i < num_domain; ++i) {
                    double curr_target;
                    curr_policy.target(GEOPM_REGION_ID_OUTER, i, curr_target);
                    double split_budget = policy_msg.power_budget * (curr_target / m_last_power_budget);
                    curr_policy.update(GEOPM_REGION_ID_OUTER, i, split_budget);
                }
            }
            else {
                // Split the budget up evenly to start.
                double split_budget = policy_msg.power_budget / num_domain;
                std::vector<double> domain_budget(num_domain);
                std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
                curr_policy.update(GEOPM_REGION_ID_OUTER, domain_budget);
            }
            m_last_power_budget = policy_msg.power_budget;
            result = true;
        }
        return result;
    }

    bool BalancingDecider::update_policy(Region &curr_region, Policy &curr_policy)
    {
        bool is_updated = false;

        // Don't do anything if we have already converged.
        if (!curr_policy.is_converged(curr_region.identifier())) {
            int num_domain = curr_policy.num_domain();
            std::vector<double> runtime(num_domain);
            double sum = 0.0;
            double sum_sqr = 0.0;
            for (int i = 0; i < num_domain; ++i) {
                runtime[i] = curr_region.median(i, GEOPM_SAMPLE_TYPE_RUNTIME);
                sum += runtime[i];
                sum_sqr += pow(runtime[i], 2);
            }
            double stddev = sqrt(sum_sqr / num_domain - pow(sum / num_domain, 2));
            // We are not within bounds. Redistribute power.
            if (stddev > m_convergence_target) {
                m_num_converged = 0;
                double total = 0.0;
                std::vector<double> percentage(num_domain);
                // Calculate new percentages
                for (int i = 0; i < num_domain; ++i) {
                    double curr_target;
                    curr_policy.target(GEOPM_REGION_ID_OUTER, i, curr_target);
                    double last_percentage = curr_target / m_last_power_budget;
                    percentage[i] = (curr_region.median(i, GEOPM_SAMPLE_TYPE_RUNTIME) * last_percentage) / sum;
                    total += percentage[i];
                }
                for (int i = 0; i < num_domain; ++i) {
                    // Re-normalize the percentages and set new target.
                    double target = (percentage[i] / total) * m_last_power_budget;
                    curr_policy.update(GEOPM_REGION_ID_OUTER, i, target);
                }
                is_updated = true;
            }
            // We are within bounds.
            else {
                m_num_converged++;
                if (m_num_converged >= m_min_num_converged) {
                    curr_policy.is_converged(curr_region.identifier(), true);
                }
            }
        }

        return is_updated;
    }
}
