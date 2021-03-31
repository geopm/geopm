/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "PowerGovernorAgent.hpp"

#include <cfloat>
#include <cmath>
#include <algorithm>

#include "PowerGovernor.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"
#include "Agg.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerGovernorAgent::PowerGovernorAgent()
        : PowerGovernorAgent(platform_io(), platform_topo(), nullptr)
    {

    }

    PowerGovernorAgent::PowerGovernorAgent(PlatformIO &platform_io, const PlatformTopo &platform_topo, std::unique_ptr<PowerGovernor> power_gov)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_level(-1)
        , m_is_converged(false)
        , m_is_sample_stable(false)
        , m_do_send_sample(false)
        , m_min_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0))
        , m_max_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0))
        , m_tdp_power_setting(m_platform_io.read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0))
        , m_power_gov(std::move(power_gov))
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_agg_func(M_NUM_SAMPLE)
        , m_num_children(0)
        , m_last_power_budget(NAN)
        , m_power_budget_changed(false)
        , m_epoch_power_buf(geopm::make_unique<CircularBuffer<double> >(16)) // Magic number...
        , m_sample(M_PLAT_NUM_SIGNAL)
        , m_ascend_count(0)
        , m_ascend_period(10)
        , m_min_num_converged(15)
        , m_adjusted_power(0.0)
        , m_last_wait(time_zero())
        , M_WAIT_SEC(0.005)
    {
        geopm_time(&m_last_wait);
    }

    PowerGovernorAgent::~PowerGovernorAgent() = default;

    void PowerGovernorAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        if (level < 0 || level > (int)fan_in.size()) {
            throw Exception("PowerGovernorAgent::init(): invalid level for given fan_in.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_level = level;
        if (m_level == 0) {
            if (nullptr == m_power_gov) {
                m_power_gov = PowerGovernor::make_unique();
            }
            init_platform_io(); // Only do this at the leaf level.
        }

        if (level == 0) {
            m_num_children = 0;
        }
        else {
            m_num_children = fan_in[level - 1];
        }

        // Setup sample aggregation for data going up the tree
        m_agg_func[M_SAMPLE_POWER] = Agg::average;
        m_agg_func[M_SAMPLE_IS_CONVERGED] = Agg::logical_and;
        m_agg_func[M_SAMPLE_POWER_ENFORCED] = Agg::average;
    }

    void PowerGovernorAgent::init_platform_io(void)
    {
        m_power_gov->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_PKG_POWER] = m_platform_io.push_signal("POWER_PACKAGE", GEOPM_DOMAIN_BOARD, 0);

        // Setup controls
        int pkg_pwr_domain_type = m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT");
        if (pkg_pwr_domain_type == GEOPM_DOMAIN_INVALID) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): Platform does not support package power control",
                            GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
        }
    }

    void PowerGovernorAgent::validate_policy(std::vector<double> &policy) const
    {
        // If NAN, use default
        if (std::isnan(policy[M_POLICY_POWER])) {
            policy[M_POLICY_POWER] = m_tdp_power_setting;
        }
        // Clamp at min and max
        if (policy[M_POLICY_POWER] < m_min_power_setting) {
            policy[M_POLICY_POWER] = m_min_power_setting;
        }
        else if (policy[M_POLICY_POWER] > m_max_power_setting) {
            policy[M_POLICY_POWER] = m_max_power_setting;
        }
    }

    void PowerGovernorAgent::split_policy(const std::vector<double> &in_policy,
                                          std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): number of policies was different from expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_level == 0) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): level 0 agent not expected to call descend.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (out_policy.size() != (size_t)m_num_children) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): policy_out vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double power_budget_in = in_policy[M_POLICY_POWER];

        if (power_budget_in > m_max_power_setting ||
            power_budget_in < m_min_power_setting) {
            throw Exception("PowerGovernorAgent::split_policy(): "
                            "invalid power budget.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Send down if this is the first budget, or if the budget changed
        if ((std::isnan(m_last_power_budget) && !std::isnan(power_budget_in)) ||
            m_last_power_budget != power_budget_in) {

            m_last_power_budget = power_budget_in;
            // Convert power budget vector into a vector of policy vectors
            for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
                out_policy[child_idx][M_POLICY_POWER] = power_budget_in;
            }
            m_epoch_power_buf->clear();
            m_is_converged = false;
            m_power_budget_changed = true;
        }
        else {
            m_power_budget_changed = false;
        }
    }

    bool PowerGovernorAgent::do_send_policy(void) const
    {
        return m_power_budget_changed;
    }

    void PowerGovernorAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                              std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_level == 0) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): level 0 agent not expected to call ascend.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (in_sample.size() != (size_t)m_num_children) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): in_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_is_sample_stable = std::all_of(in_sample.begin(), in_sample.end(),
            [](const std::vector<double> &val)
            {
                return val[M_SAMPLE_IS_CONVERGED];
            });

        // If all children report that they are converged for the last
        // ascend period times, then aggregate the samples and send
        // them up the tree.
        if (m_is_sample_stable && m_ascend_count == 0) {
            m_do_send_sample = true;
            Agent::aggregate_sample(in_sample, m_agg_func, out_sample);
        }
        else {
            m_do_send_sample = false;
        }

        // Increment the ascend counter if the children are stable.
        if (m_is_sample_stable) {
            ++m_ascend_count;
            if (m_ascend_count == m_ascend_period) {
               m_ascend_count = 0;
            }
        }

    }

    bool PowerGovernorAgent::do_send_sample(void) const
    {
        return m_do_send_sample;
    }

    void PowerGovernorAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double power_budget_in = in_policy[M_POLICY_POWER];
        m_power_gov->adjust_platform(power_budget_in, m_adjusted_power);
        m_last_power_budget = power_budget_in;
    }

    bool PowerGovernorAgent::do_write_batch(void) const
    {
        return m_power_gov->do_write_batch();
    }

    void PowerGovernorAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_power_gov->sample_platform();
        // Populate sample vector by reading from PlatformIO
        for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SIGNAL; ++sample_idx) {
            m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
        }

        /// @todo should use EPOCH_ENERGY signal which doesn't currently exist
        if (!std::isnan(m_sample[M_PLAT_SIGNAL_PKG_POWER])) {
            m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_PKG_POWER]);
        }
        // If we have observed more than m_min_num_converged epoch
        // calls then send median filtered power values up the tree.
        if (m_epoch_power_buf->size() > m_min_num_converged) {
            double median = Agg::median(m_epoch_power_buf->make_vector());
            out_sample[M_SAMPLE_POWER] = median;
            out_sample[M_SAMPLE_IS_CONVERGED] = (median <= m_last_power_budget); // todo might want fudge factor
            out_sample[M_SAMPLE_POWER_ENFORCED] = m_adjusted_power;
            m_do_send_sample = true;
        }
        else {
            m_do_send_sample = false;
        }
    }

    void PowerGovernorAgent::wait()
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > PowerGovernorAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerGovernorAgent::report_host(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerGovernorAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerGovernorAgent::trace_names(void) const
    {
        return {"POWER_BUDGET"};
    }

    std::vector<std::function<std::string(double)> > PowerGovernorAgent::trace_formats(void) const
    {
        return {string_format_double};
    }

    void PowerGovernorAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget;
    }

    void PowerGovernorAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("PowerGovernorAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int control_domain = m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT");
        double pkg_policy = policy[M_POLICY_POWER] / m_platform_topo.num_domain(control_domain);
        m_platform_io.write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_BOARD, 0, pkg_policy);
    }

    std::string PowerGovernorAgent::plugin_name(void)
    {
        return "power_governor";
    }

    std::unique_ptr<Agent> PowerGovernorAgent::make_plugin(void)
    {
        return geopm::make_unique<PowerGovernorAgent>();
    }

    std::vector<std::string> PowerGovernorAgent::policy_names(void)
    {
        return {"POWER_PACKAGE_LIMIT_TOTAL"};
    }

    std::vector<std::string> PowerGovernorAgent::sample_names(void)
    {
        return {"POWER", "IS_CONVERGED", "POWER_AVERAGE_ENFORCED"};
    }
}
