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

#include "geopm_message.h"
#include "geopm_plugin.h"
#include "GoverningDecider.hpp"
#include "CPUBalancingDecider.hpp"
#include "Exception.hpp"

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::IDecider *decider = new geopm::CPUBalancingDecider;
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
    CPUBalancingDecider::CPUBalancingDecider()
        : GoverningDecider()
        , m_name("cpu_balancing")
    {

    }

    CPUBalancingDecider::CPUBalancingDecider(const CPUBalancingDecider &other)
        : GoverningDecider(other)
        , m_name(other.m_name)
    {

    }

    CPUBalancingDecider::~CPUBalancingDecider()
    {

    }

    IDecider *CPUBalancingDecider::clone(void) const
    {
        return (IDecider*)(new CPUBalancingDecider(*this));
    }

    bool CPUBalancingDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& CPUBalancingDecider::name(void) const
    {
        return m_name;
    }

    bool CPUBalancingDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        bool is_target_updated = GoverningDecider::update_policy(curr_region, curr_policy);

        std::vector<double> thread_progress;
        curr_region.thread_progress(thread_progress);
        //size_t num_cpu = thread_progress.size();

        /// @todo Would be nice to know how the thread_progress_idx
        ///       maps to control domain_idx.  This could be derived
        ///       from the hwloc_topology_t PlatformTopology::m_topo
        ///       data structure.
        uint64_t region_id = curr_region.identifier();
        int num_domain = curr_policy.num_domain();
        std::vector<double> target(num_domain);
        std::vector<double> domain_progress_rate(num_domain);
        std::vector<double> domain_freq(num_domain);
        std::vector<double> domain_freq_achieved(num_domain);
        curr_policy.target(region_id, target);
        double power_target = 0.0;
        double power_used = 0.0;
        for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
            power_used += curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
            power_used += curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
            power_target += target[domain_idx];
            domain_progress_rate[domain_idx] = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_PROGRESS);
            domain_freq[domain_idx] = curr_region.mean(domain_idx, GEOPM_TELEMETRY_TYPE_FREQUENCY);
            domain_freq_achieved[domain_idx] = curr_region.max(domain_idx, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE) -
                                               curr_region.min(domain_idx, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE);
            domain_freq_achieved[domain_idx] /= curr_region.max(domain_idx, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF) -
                                                curr_region.min(domain_idx, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF);
        }

        // CODE GOES HERE

        return is_target_updated;
    }
}
