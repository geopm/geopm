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
        , m_guard_band(0.05)
        , m_min_num_converged(5)
        , m_last_power_budget(DBL_MIN)
        , m_num_sample(5)
        , m_num_out_of_range(0)
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
        bool is_updated = false;
        bool is_greater = false;
        bool is_less = false;

        if (curr_region.num_sample(0, GEOPM_SAMPLE_TYPE_RUNTIME) > m_num_sample) {
            const int num_domain = curr_policy.num_domain();
            const uint64_t region_id = curr_region.identifier();

            std::vector<double> limit(num_domain);
            std::vector<double> target(num_domain);
            curr_policy.target(GEOPM_REGION_ID_EPOCH, limit);
            curr_policy.target(region_id, target);
            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                double pkg_power = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
                double dram_power = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
                double total_power = pkg_power + dram_power;
                is_greater = total_power > limit[domain_idx] * (1 + m_guard_band);
                is_less = total_power < limit[domain_idx] * (1 - m_guard_band);
                if (is_greater || is_less) {
                    double overage = total_power - limit[domain_idx];
                    target[domain_idx] = limit[domain_idx] - (overage > dram_power ?
                                                              overage : dram_power);
                    is_updated = true;
                }
            }
            if (is_updated && curr_policy.is_converged(region_id)) {
                ++m_num_out_of_range;
                if (m_num_out_of_range < m_min_num_converged) {
                    is_updated = false;
                }
            }
            if (is_updated && curr_policy.is_converged(region_id)) {
                curr_policy.update(region_id, target);
                if (is_greater) {
                    auto it = m_num_converged.lower_bound(region_id);
                    if (it != m_num_converged.end() && (*it).first == region_id) {
                        (*it).second = 0;
                    }
                    else {
                        it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 0));
                    }
                    curr_policy.is_converged(region_id, false);
                    curr_region.clear();
                    m_num_sample = 0;
                    m_num_out_of_range = 0;
                }
            }
            if (!is_updated || is_less) {
                if (curr_policy.is_converged(region_id)) {
                    m_num_out_of_range = 0;
                }
                auto it = m_num_converged.lower_bound(region_id);
                if (it != m_num_converged.end() && (*it).first == region_id) {
                    ++(*it).second;
                }
                else {
                    it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 1));
                }
                if ((*it).second >= m_min_num_converged) {
                    curr_policy.is_converged(region_id, true);
                }
            }
        }
        return is_updated;
    }
}
