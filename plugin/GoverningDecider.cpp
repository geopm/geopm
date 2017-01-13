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
            geopm::IDecider *decider = new geopm::GoverningDecider;
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
        , m_last_dram_power(DBL_MAX)
        , m_num_sample(5)
    {

    }

    GoverningDecider::GoverningDecider(const GoverningDecider &other)
        : Decider(other)
        , m_name(other.m_name)
        , m_min_num_converged(other.m_min_num_converged)
        , m_last_power_budget(other.m_last_power_budget)
        , m_last_dram_power(other.m_last_dram_power)
        , m_num_sample(other.m_num_sample)
    {

    }

    GoverningDecider::~GoverningDecider()
    {

    }

    IDecider *GoverningDecider::clone(void) const
    {
        return (IDecider*)(new GoverningDecider(*this));
    }

    bool GoverningDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& GoverningDecider::name(void) const
    {
        return m_name;
    }

    void GoverningDecider::requires(int level, TelemetryConfig &config)
    {
        if (!config.is_domain_supported(GEOPM_DOMAIN_CONTROL_POWER)) {
            throw Exception("GoverningDecider::enable_features(): platform does not support required control type: " +
                            "GEOPM_DOMAIN_CONTROL_POWER",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!config.is_provided(GEOPM_DOMAIN_SIGNAL_ENERGY, "dram_energy")) {
            throw Exception("GoverningDecider::enable_features(): platform does not support required signal type: " +
                            "dram_energy",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        config.set_required(GEOPM_DOMAIN_SIGNAL_ENERGY, "dram_energy");
    }

    bool GoverningDecider::update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy)
    {
        bool result = false;
        if (policy_msg.power_budget != m_last_power_budget) {
            int num_domain = curr_policy.num_control_domain();
            double split_budget = policy_msg.power_budget / num_domain;
            std::vector<double> domain_budget(num_domain);
            std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
            std::vector<uint64_t> region_id;
            curr_policy.region_id(region_id);
            for (auto region = region_id.begin(); region != region_id.end(); ++region) {
                curr_policy.update((*region), GEOPM_CONTROL_TYPE_POWER, domain_budget);
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
            m_last_dram_power = DBL_MAX;
            result = true;
        }
        return result;
    }

    bool GoverningDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        static const double GUARD_BAND = 0.02;
        bool is_target_updated = false;
        const uint64_t region_id = curr_region.identifier();

        // If we have enough samples from the current region then update policy.
        if (curr_region.num_sample(0, GEOPM_TELEMETRY_TYPE_PKG_ENERGY) >= m_num_sample) {
            const int num_domain = curr_policy.num_control_domain();
            std::vector<double> limit(num_domain);
            std::vector<double> target(num_domain);
            std::vector<double> domain_dram_power(num_domain);
            // Get node limit for epoch set by tree decider
            curr_policy.target(GEOPM_REGION_ID_EPOCH, GEOPM_CONTROL_TYPE_POWER, limit);
            // Get last policy target for the current region
            curr_policy.target(region_id, GEOPM_CONTROL_TYPE_POWER, target);

            // Sum package and dram power over all domains to get total_power
            double dram_power = 0.0;
            double limit_total = 0.0;
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                domain_dram_power[domain_idx] = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
                dram_power += domain_dram_power[domain_idx];
                limit_total += limit[domain_idx];
            }
            double upper_limit = m_last_dram_power + (GUARD_BAND * limit_total);
            double lower_limit = m_last_dram_power - (GUARD_BAND * limit_total);

            // If we have enough energy samples to accurately
            // calculate power: derivative function did not return NaN.
            if (!std::isnan(dram_power)) {
                if (dram_power < lower_limit || dram_power > upper_limit) {
                    m_last_dram_power = dram_power;
                    for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                        target[domain_idx] = limit[domain_idx] - domain_dram_power[domain_idx];
                    }
                    curr_policy.update(region_id, GEOPM_CONTROL_TYPE_POWER, target);
                    is_target_updated = true;
                }
                if (!curr_policy.is_converged(region_id)) {
                    if (is_target_updated) {
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
            }
        }

        return is_target_updated;
    }
}
