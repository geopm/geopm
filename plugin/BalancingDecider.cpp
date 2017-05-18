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
#include <algorithm>

#include "geopm_message.h"
#include "geopm_plugin.h"
#include "BalancingDecider.hpp"
#include "Exception.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::IDecider *decider = new geopm::BalancingDecider;
            geopm_factory_register(factory, decider, dl_ptr);
        }
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception());
    }

    return err;
}

struct {
    bool operator()(std::pair<int,double> a, std::pair<int,double> b)
    {
        return a.second < b.second;
    }
} pair_greater;

namespace geopm
{
    BalancingDecider::BalancingDecider()
        : m_name("power_balancing")
        , m_convergence_target(0.01)
        , m_min_num_converged(7)
        , m_num_converged(0)
        , m_last_power_budget(DBL_MIN)
        , m_num_sample(3)
        , m_num_out_of_range(0)
        , m_slope_modifier(3.0)
        , M_GUARD_BAND(1.15)
    {

    }

    BalancingDecider::BalancingDecider(const BalancingDecider &other)
        : Decider(other)
        , m_name(other.m_name)
        , m_convergence_target(other.m_convergence_target)
        , m_min_num_converged(other.m_min_num_converged)
        , m_num_converged(other.m_num_converged)
        , m_last_power_budget(other.m_last_power_budget)
        , m_num_sample(other.m_num_sample)
        , m_num_out_of_range(other.m_num_out_of_range)
        , m_slope_modifier(other.m_slope_modifier)
        , M_GUARD_BAND(other.M_GUARD_BAND)
    {

    }

    BalancingDecider::~BalancingDecider()
    {

    }

    IDecider *BalancingDecider::clone(void) const
    {
        return (IDecider*)(new BalancingDecider(*this));
    }

    bool BalancingDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& BalancingDecider::name(void) const
    {
        return m_name;
    }

    void BalancingDecider::bound(double upper_bound, double lower_bound)
    {
        m_upper_bound = upper_bound / M_GUARD_BAND;
        m_lower_bound = lower_bound * M_GUARD_BAND;
    }

    bool BalancingDecider::update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy)
    {
        bool result = false;
        if (policy_msg.power_budget != m_last_power_budget) {
            curr_policy.is_converged(GEOPM_REGION_ID_EPOCH, false);
            int num_domain = curr_policy.num_domain();
            if (m_last_power_budget != DBL_MIN) {
                // Split the budget up by the same ratio we split the old budget.
                for (int i = 0; i < num_domain; ++i) {
                    double curr_target;
                    curr_policy.target(GEOPM_REGION_ID_EPOCH, i, curr_target);
                    double split_budget = policy_msg.power_budget * (curr_target / m_last_power_budget);
                    curr_policy.update(GEOPM_REGION_ID_EPOCH, i, split_budget);
                }
            }
            else {
                // Split the budget up evenly to start.
                double split_budget = policy_msg.power_budget / num_domain;
                std::vector<double> domain_budget(num_domain);
                std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
                curr_policy.update(GEOPM_REGION_ID_EPOCH, domain_budget);
            }
            m_last_power_budget = policy_msg.power_budget;
            result = true;
        }
        return result;
    }

    bool BalancingDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        bool is_updated = false;

        // Don't do anything if we have already converged.
        if (curr_region.num_sample(0, GEOPM_SAMPLE_TYPE_RUNTIME) >= m_num_sample) {
            int num_domain = curr_policy.num_domain();
            std::vector<std::pair<int,double> > runtime(num_domain);
            double sum = 0.0;
            double sum_sqr = 0.0;
            for (int i = 0; i < num_domain; ++i) {
                runtime[i].first = i;
                runtime[i].second = curr_region.median(i, GEOPM_SAMPLE_TYPE_RUNTIME);
                sum += runtime[i].second;
                sum_sqr += runtime[i].second * runtime[i].second;
            }
            double mean = sum / num_domain;
            double rel_stddev = sqrt(sum_sqr / num_domain - (mean * mean)) / mean;
            // We are not within bounds. Redistribute power.
            if (!curr_policy.is_converged(curr_region.identifier()) && (rel_stddev > m_convergence_target)) {
                m_num_converged = 0;
                double total = 0.0;
                std::vector<double> percentage(num_domain);
                std::sort(runtime.begin(), runtime.end(), pair_greater);
                for (auto iter = runtime.begin(); iter != runtime.end(); ++iter) {
                    double median;
                    double curr_target;
                    curr_policy.target(GEOPM_REGION_ID_EPOCH, (*iter).first, curr_target);
                    double last_percentage = curr_target / m_last_power_budget;
                    median = curr_region.median((*iter).first, GEOPM_SAMPLE_TYPE_RUNTIME);
                    percentage[(*iter).first] = (((mean * m_slope_modifier) + median) * last_percentage) / sum;
                    total += percentage[(*iter).first];
                }

                int pool = m_last_power_budget;
                int power_sum = 0;
                double runtime_sum = 0.0;
                for (auto iter = runtime.begin(); iter != runtime.end(); ++iter) {
                    double target = (percentage[(*iter).first] / total) * pool;
                    if (target < m_lower_bound) {
                        target = m_lower_bound;
                        pool -= (target + power_sum);
                        sum -= (curr_region.median((*iter).first, GEOPM_SAMPLE_TYPE_RUNTIME) + runtime_sum);
                        power_sum = 0;
                        runtime_sum = 0.0;
                        total = 0.0;
                        for (auto it = (iter + 1); it != runtime.end(); ++it) {
                            double median;
                            double curr_target;
                            curr_policy.target(GEOPM_REGION_ID_EPOCH, (*it).first, curr_target);
                            double last_percentage = curr_target / m_last_power_budget;
                            median = curr_region.median((*it).first, GEOPM_SAMPLE_TYPE_RUNTIME);
                            percentage[(*it).first] = (((mean * m_slope_modifier) + median) * last_percentage) / sum;
                            total += percentage[(*it).first];
                        }
                    }
                    else {
                        power_sum += target;
                        runtime_sum += curr_region.median((*iter).first, GEOPM_SAMPLE_TYPE_RUNTIME);
                    }
                    curr_policy.update(GEOPM_REGION_ID_EPOCH, (*iter).first, target);
                }
                // clear out stale sample data
                curr_region.clear();
                is_updated = true;
            }
            if (curr_policy.is_converged(curr_region.identifier()) && (rel_stddev > m_convergence_target)) {
                ++m_num_out_of_range;
                if (m_num_out_of_range >= m_min_num_converged) {
                    curr_policy.is_converged(curr_region.identifier(), false);
                    m_num_converged = 0;
                    m_num_out_of_range = 0;
                }
            }
            // We are within bounds.
            else if (!curr_policy.is_converged(curr_region.identifier()) && (rel_stddev < m_convergence_target)) {
                m_num_out_of_range = 0;
                ++m_num_converged;
                if (m_num_converged >= m_min_num_converged) {
                    curr_policy.is_converged(curr_region.identifier(), true);
                }
            }
        }

        return is_updated;
    }
}
