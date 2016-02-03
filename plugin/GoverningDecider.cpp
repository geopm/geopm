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

    bool GoverningDecider::update_policy(Region &curr_region, Policy &curr_policy)
    {
#if 0
        double package_power = 0.0;
        double memory_power = 0.0;
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
#endif
        return false;
    }
}
