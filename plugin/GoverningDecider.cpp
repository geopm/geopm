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
#include "GoverningDecider.hpp"
#include "Exception.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::Decider *decider = new geopm::GoverningDecider;
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
    GoverningDecider::GoverningDecider()
        : m_name("governing")
        , m_guard_band(0.05)
        , m_min_num_converged(5)
        , m_last_power_budget(DBL_MIN)
    {

    }

    GoverningDecider::~GoverningDecider()
    {

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
            for (auto it = region_id.begin(); it != region_id.end(); ++it) {
                curr_policy.update(*it, domain_budget);
            }
            m_last_power_budget = policy_msg.power_budget;
            result = true;
        }
        return result;
    }

    bool GoverningDecider::update_policy(Region &curr_region, Policy &curr_policy)
    {
        const int num_domain = curr_policy.num_domain();
        const uint64_t region_id = curr_region.identifier();
        bool is_updated = false;

        std::vector<double> target(num_domain);
        curr_policy.target(region_id, target);
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            double pkg_power = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
            double dram_power = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
            double total_power = pkg_power + dram_power;
            if (total_power > target[domain_idx] * (1 + m_guard_band) ||
                total_power < target[domain_idx] * (1 - m_guard_band)) {
                target[domain_idx] -= dram_power;
                is_updated = true;
            }
        }
        if (is_updated) {
            curr_policy.update(region_id, target);
            auto it = m_num_converged.lower_bound(region_id);
            if (it != m_num_converged.end() && (*it).first == region_id) {
                (*it).second = 0;
            }
            else {
                it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 0));
            }
            curr_policy.is_converged(region_id, false);
        }
        else {
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
        return is_updated;
    }
}
