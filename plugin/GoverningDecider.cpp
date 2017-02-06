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
#include <cmath>
#include "geopm_message.h"
#include "geopm_plugin.h"
#include "GoverningDecider.hpp"
#include "Exception.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::Decider *decider = new geopm::GoverningDecider;
            geopm_factory_register(factory, decider, dl_ptr);
        }
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception());
    }

    return err;
}

namespace geopm
{
    GoverningDecider::GoverningDecider()
        : m_name("power_governing")
        , m_min_num_converged(5)
        , m_last_power_budget(DBL_MIN)
        , m_num_sample(5)
    {

    }

    GoverningDecider::GoverningDecider(const GoverningDecider &other)
        : Decider(other)
        , m_name(other.m_name)
        , m_min_num_converged(other.m_min_num_converged)
        , m_last_power_budget(other.m_last_power_budget)
        , m_num_sample(other.m_num_sample)
    {

    }

    GoverningDecider::~GoverningDecider()
    {

    }

    Decider *GoverningDecider::clone(void) const
    {
        return (Decider*)(new GoverningDecider(*this));
    }

    bool GoverningDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& GoverningDecider::name(void) const
    {
        return m_name;
    }

    bool GoverningDecider::update_policy(const struct geopm_policy_message_s &policy_msg, Policy &curr_policy)
    {
        bool result = false;
        if (policy_msg.power_budget != m_last_power_budget) {
            int num_domain = curr_policy.num_domain();
            double split_budget = policy_msg.power_budget / num_domain;
            std::vector<double> domain_budget(num_domain);
            std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
            std::vector<uint64_t> region_id;
            curr_policy.region_id(region_id);
            for (auto region = region_id.begin(); region != region_id.end(); ++region) {
                curr_policy.update((*region), domain_budget);
                auto it = m_num_converged.lower_bound((*region));
                if (it != m_num_converged.end() && (*it).first == (*region)) {
                    (*it).second = 0;
                }
                else {
                    it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>((*region), 0));
                }
                curr_policy.is_converged((*region), false);
            }
            if (m_last_power_budget == DBL_MIN) {
                curr_policy.mode(policy_msg.mode);
                curr_policy.policy_flags(policy_msg.flags);
            }
            m_last_power_budget = policy_msg.power_budget;
            result = true;
        }
        return result;
    }

    bool GoverningDecider::update_policy(Region &curr_region, Policy &curr_policy)
    {
        static const double UPPER_GUARD_BAND = 1.05;
        static const double LOWER_GUARD_BAND = 0.99;
        bool is_target_updated = false;
        bool is_greater = false;

        // If we have enough samples from the current region then update policy.
        if (curr_region.num_sample(0, GEOPM_TELEMETRY_TYPE_PKG_ENERGY) >= m_num_sample) {
            const int num_domain = curr_policy.num_domain();
            const uint64_t region_id = curr_region.identifier();
            std::vector<double> limit(num_domain);
            std::vector<double> target(num_domain);
            std::vector<double> domain_pkg_power(num_domain);
            std::vector<double> domain_dram_power(num_domain);
            // Get node limit for epoch set by tree decider
            curr_policy.target(GEOPM_REGION_ID_EPOCH, limit);
            // Get last policy target for the current region
            curr_policy.target(region_id, target);

            // Sum package and dram power over all domains to get total_power
            double pkg_power = 0.0;
            double dram_power = 0.0;
            double limit_total = 0.0;
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                domain_pkg_power[domain_idx] = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
                pkg_power += domain_pkg_power[domain_idx];
                domain_dram_power[domain_idx] = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
                dram_power += domain_dram_power[domain_idx];
                limit_total += limit[domain_idx];
            }
            double total_power = pkg_power + dram_power;

            // If we have enough energy samples to accurately
            // calculate power: derivative function did not return NaN.
            if (!std::isnan(total_power)) {
                is_greater = total_power > limit_total * UPPER_GUARD_BAND;
                double overage = total_power - limit_total;
                double target_total = limit_total * LOWER_GUARD_BAND - (overage > dram_power ?
                                                                        overage : dram_power);

                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    // Divide the total power budget over domains
                    // in proportion to the fraction of power used by
                    // each domain as observed in the last sample.
                    target[domain_idx] = target_total * (domain_pkg_power[domain_idx] +
                                         domain_dram_power[domain_idx]) / total_power;
                }

                if (!curr_policy.is_converged(region_id)) {
                    // If the region's policy is currently in a state
                    // of "unconvergence", update the target
                    curr_policy.update(region_id, target);
                    is_target_updated = true;
                    if (is_greater) {
                        // Set to zero the number of times we were
                        // under budget since last in "converged"
                        // state (since policy is not currently in a
                        // converged state).
                        auto it = m_num_converged.lower_bound(region_id);
                        if (it != m_num_converged.end() && (*it).first == region_id) {
                            (*it).second = 0;
                        }
                        else {
                            it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 0));
                        }
                    }
                    else {
                        // If the region's policy is in a state of
                        // "unconvergence", but the node is currently
                        // under the target budget increment the
                        // number of samples under budget since last
                        // last convergence or last overage.
                        auto it = m_num_converged.lower_bound(region_id);
                        if (it != m_num_converged.end() && (*it).first == region_id) {
                            ++(*it).second;
                        }
                        else {
                            it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 1));
                        }
                        if ((*it).second >= m_min_num_converged) {
                            // If we have observed m_min_num_converged
                            // samples in a row under budget set the
                            // region to the "converged" state and
                            // reset the counter.
                            curr_policy.is_converged(region_id, true);
                            (*it).second = 0;
                        }
                    }
                }
                else {
                    // If the region's policy is currently is a state
                    // of "convergence" count number of samples where
                    // we go over budget since convergence occured.
                    if (is_greater) {
                        auto it = m_num_converged.lower_bound(region_id);
                        if (it != m_num_converged.end() && (*it).first == region_id) {
                            ++(*it).second;
                        }
                        else {
                            it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 1));
                        }
                        if ((*it).second >= m_min_num_converged) {
                            // As soon as m_min_num_converged samples
                            // are found over the budget since
                            // convergence occured switch policy state
                            // to "unconverged" and reset counter.
                            curr_policy.is_converged(region_id, false);
                            (*it).second = 0;
                        }
                    }
                }
                if (is_target_updated) {
                    // If region policy was in an unconverged state and
                    // we were able to measure power, remove all data
                    // from region.
                    curr_region.clear();
                }
            }
        }

        /// @fixme: It is much easier to unconverge than converge:
        /// Counter toward convergence is reset to zero every time you
        /// go under the budget if policy is in unconverged state, but
        /// if policy is in converged state the counter towards
        /// unconverged state keeps growing until it exceedes limit
        /// and state is switched to to unconverged.

        /// @fixme: Why clear all of the region data (including the
        /// data used to measure power) every time we are able to
        /// measure power and we are in an unconverged state?

        return is_target_updated;
    }
}
