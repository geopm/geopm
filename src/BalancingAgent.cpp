/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <cmath>

#include "BalancingAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    BalancingAgent::BalancingAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_convergence_guard_band(0.5)
        , m_level(-1)
        , m_num_children(0)
        , m_power_budget_in_idx(-1)
        , m_power_used_out_idx(-1)
        , m_runtime_out_idx(-1)
        , m_power_used_in_idx(-1)
        , m_runtime_in_idx(-1)
        , m_is_converged(false)
    {

    }

    BalancingAgent::~BalancingAgent()
    {

    }

    void BalancingAgent::init(int level, int num_leaf)
    {
        m_level = level;
        if (m_level == 0) {
            m_num_children = 1;
            // Push hardware controls
            m_power_budget_out_idx.push_back(m_platform_io.push_control("POWER", IPlatformTopo::M_DOMAIN_BOARD, 0));
            // Push hardware signals
            m_power_used_agg_in_idx = m_platform_io.push_signal("POWER", IPlatformTopo::M_DOMAIN_BOARD, 0);
            m_runtime_in_idx.push_back(m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0));
            // At leaf level, use total values for split_budget (i.e. this node is only child)
            m_power_used_in_idx.push_back(m_power_used_agg_in_idx);
            m_runtime_in_idx.push_back(m_runtime_agg_in_idx);
        }
        else {
            // Setup messages with children
            m_num_children = -1; // todo
        }
        m_is_converged_in.resize(m_num_children);
        m_runtime_in.resize(m_num_children, NAN);
        m_power_used_in.resize(m_num_children, NAN);
        m_power_budget_out.resize(m_num_children, NAN);
        m_agg_fn_is_converged = IPlatformIO::agg_and;
        m_agg_fn_power = m_platform_io.agg_function("POWER");
        m_agg_fn_epoch_runtime = m_platform_io.agg_function("EPOCH_RUNTIME");
    }

    bool BalancingAgent::descend(const std::vector<double> &in_message, std::vector<std::vector<double> >&out_message)
    {
        if (m_is_converged) {
            double power_budget_in = 100;//m_platform_io.sample(m_power_budget_in_idx);
            split_budget(power_budget_in, m_power_used_in, m_runtime_in, m_power_budget_out);
            for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
                m_platform_io.adjust(m_power_budget_out_idx[child_idx], m_power_budget_out[child_idx]);
            }
        }
        return false;
    }

    bool BalancingAgent::ascend(const std::vector<std::vector<double> > &in_message, std::vector<double> &out_message)
    {
        // Read samples from children or from the platform.
        for (int child_idx = 0; child_idx < m_num_children; ++child_idx) {
            m_power_used_in[child_idx] = m_platform_io.sample(m_power_used_in_idx[child_idx]);
            m_runtime_in[child_idx] = m_platform_io.sample(m_runtime_in_idx[child_idx]);
        }

        // Send aggregated signals up the tree.
        m_platform_io.adjust(m_power_used_out_idx,
                             m_platform_io.sample(m_power_used_agg_in_idx));
        m_platform_io.adjust(m_runtime_out_idx,
                             m_platform_io.sample(m_runtime_agg_in_idx));
        if (m_level) {
            m_is_converged = m_platform_io.sample(m_is_converged_agg_in_idx) != 0.0;
        }
        else {
            m_is_converged = true;
            for (int child_idx = 0; m_is_converged && child_idx != m_num_children; ++child_idx) {
                if (m_power_used_in[child_idx] > (1.0 + m_convergence_guard_band) * m_power_budget_out[child_idx]) {
                    m_is_converged = false;
                }
            }
        }
        m_platform_io.adjust(m_is_converged_out_idx, m_is_converged);
        return false;
    }

    void BalancingAgent::adjust_platform(const std::vector<double> &policy)
    {

    }

    bool BalancingAgent::sample_platform(std::vector<double> &sample)
    {
        return false;
    }

    void BalancingAgent::wait()
    {

    }

    void BalancingAgent::split_budget(double total_power_budget,
                                      const std::vector<double> &power_used,
                                      const std::vector<double> &runtime,
                                      std::vector<double> &result)
    {
#ifdef GEOPM_DEBUG
        if (power_used.size() != runtime.size() ||
            power_used.size() != result.size()) {
            throw Exception("BalancingAgent::split_budget(): input vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    std::string BalancingAgent::report_header(void)
    {
        return "balancing agent";
    }

    std::string BalancingAgent::report_node(void)
    {
        return "";
    }

    std::map<uint64_t, std::string> BalancingAgent::report_region(void)
    {
        return {};
    }

    std::vector<std::string> BalancingAgent::trace_names(void) const
    {
        return {};
    }

    void BalancingAgent::trace_values(std::vector<double> &values)
    {
        ///@todo for each trace column, sample and fill in values
    }

    std::string BalancingAgent::plugin_name(void)
    {
        return "BALANCING";
    }

    std::unique_ptr<IAgent> BalancingAgent::make_plugin(void)
    {
        return geopm::make_unique<BalancingAgent>();
    }

    std::vector<std::string> BalancingAgent::policy_names(void)
    {
        return {};
    }

    std::vector<std::string> BalancingAgent::sample_names(void)
    {
        return {};
    }
}
