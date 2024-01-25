/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "NodePowerGovernorAgent.hpp"

#include <cfloat>
#include <cmath>
#include <algorithm>

#include "PowerGovernor.hpp"
#include "PlatformIOProf.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/CircularBuffer.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "config.h"

namespace geopm
{
    NodePowerGovernorAgent::NodePowerGovernorAgent()
        : NodePowerGovernorAgent(PlatformIOProf::platform_io())
    {

    }

    NodePowerGovernorAgent::NodePowerGovernorAgent(PlatformIO &platform_io)
        : m_platform_io(platform_io)
        , m_level(-1)
        , m_is_sample_stable(false)
        , m_do_send_sample(false)
        // Placeholder for min/max signal equivalents to existing PowerGovernor
        , M_MIN_POWER_SETTING(0)
        , M_MAX_POWER_SETTING(std::numeric_limits<double>::max())
        , M_POWER_TIME_WINDOW(0.013)
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_pio_ctl_idx(M_PLAT_NUM_CONTROL)
        , m_agg_func(M_NUM_SAMPLE)
        , m_num_children(0)
        , m_do_write_batch(false)
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

    NodePowerGovernorAgent::~NodePowerGovernorAgent() = default;

    void NodePowerGovernorAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        if (level < 0 || level > (int)fan_in.size()) {
            throw Exception("NodePowerGovernorAgent::init(): invalid level for given fan_in.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_level = level;
        if (m_level == 0) {
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

    void NodePowerGovernorAgent::init_platform_io(void)
    {
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_NODE_POWER] = m_platform_io.push_signal("MSR::BOARD_POWER", GEOPM_DOMAIN_BOARD, 0);

        // Check support for Platform Energy
        double platform_energy = m_platform_io.read_signal("MSR::BOARD_ENERGY", GEOPM_DOMAIN_BOARD, 0);
        if (platform_energy == 0) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) +
                            "(): Platform does not support platform energy.",
                            GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
        }

        // Setup controls
        const std::string CONTROL_NAME = "MSR::PLATFORM_POWER_LIMIT:PL1_POWER_LIMIT";
        int node_pwr_domain_type = m_platform_io.control_domain_type(CONTROL_NAME);
        if (node_pwr_domain_type == GEOPM_DOMAIN_INVALID) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) +
                            "(): Platform does not support platform power control.",
                            GEOPM_ERROR_AGENT_UNSUPPORTED, __FILE__, __LINE__);
        }

        m_pio_ctl_idx[M_PLAT_CONTROL_NODE_POWER] = m_platform_io.push_control(CONTROL_NAME, GEOPM_DOMAIN_BOARD, 0);

        // Setup time window and enable feature
        m_platform_io.write_control("MSR::PLATFORM_POWER_LIMIT:PL1_TIME_WINDOW", GEOPM_DOMAIN_BOARD, 0, M_POWER_TIME_WINDOW);
        m_platform_io.write_control("MSR::PLATFORM_POWER_LIMIT:PL1_LIMIT_ENABLE", GEOPM_DOMAIN_BOARD, 0, 1);
        m_platform_io.write_control("MSR::PLATFORM_POWER_LIMIT:PL1_CLAMP_ENABLE", GEOPM_DOMAIN_BOARD, 0, 1);
    }

    void NodePowerGovernorAgent::validate_policy(std::vector<double> &policy) const
    {
        // If NAN, throw
        if (std::isnan(policy[M_POLICY_POWER])) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): policy cannot be NAN.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Clamp at min and max
        if (policy[M_POLICY_POWER] < M_MIN_POWER_SETTING) {
            policy[M_POLICY_POWER] = M_MIN_POWER_SETTING;
        }
        else if (policy[M_POLICY_POWER] > M_MAX_POWER_SETTING) {
            policy[M_POLICY_POWER] = M_MAX_POWER_SETTING;
        }
    }

    void NodePowerGovernorAgent::split_policy(const std::vector<double> &in_policy,
                                          std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): number of policies was different from expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_level == 0) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): level 0 agent not expected to call descend.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (out_policy.size() != (size_t)m_num_children) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): policy_out vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double power_budget_in = in_policy[M_POLICY_POWER];

        if (power_budget_in > M_MAX_POWER_SETTING ||
            power_budget_in < M_MIN_POWER_SETTING) {
            throw Exception("NodePowerGovernorAgent::split_policy(): "
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
            m_power_budget_changed = true;
        }
        else {
            m_power_budget_changed = false;
        }
    }

    bool NodePowerGovernorAgent::do_send_policy(void) const
    {
        return m_power_budget_changed;
    }

    void NodePowerGovernorAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                              std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_level == 0) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): level 0 agent not expected to call ascend.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (in_sample.size() != (size_t)m_num_children) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): in_sample vector not correctly sized.",
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

    bool NodePowerGovernorAgent::do_send_sample(void) const
    {
        return m_do_send_sample;
    }

    void NodePowerGovernorAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double power_budget_in = in_policy[M_POLICY_POWER];

        m_do_write_batch = false;
        if (!std::isnan(power_budget_in)) {
            if (power_budget_in < M_MIN_POWER_SETTING) {
                power_budget_in = M_MIN_POWER_SETTING;
            }
            else if (power_budget_in > M_MAX_POWER_SETTING) {
                power_budget_in = M_MAX_POWER_SETTING;
            }

            if (m_last_power_budget != power_budget_in) {
                for (int ctl_idx = 0; ctl_idx < M_PLAT_NUM_CONTROL; ++ctl_idx) {
                    m_platform_io.adjust(ctl_idx, power_budget_in);
                }
                m_last_power_budget = power_budget_in;
                m_do_write_batch = true;
            }
        }

        m_last_power_budget = power_budget_in;
    }

    bool NodePowerGovernorAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    void NodePowerGovernorAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        // Populate sample vector by reading from PlatformIO
        for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SIGNAL; ++sample_idx) {
            m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
        }

        /// @todo should use EPOCH_ENERGY signal which doesn't currently exist
        if (!std::isnan(m_sample[M_PLAT_SIGNAL_NODE_POWER])) {
            m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_NODE_POWER]);
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

    void NodePowerGovernorAgent::wait()
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > NodePowerGovernorAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > NodePowerGovernorAgent::report_host(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > NodePowerGovernorAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> NodePowerGovernorAgent::trace_names(void) const
    {
        return {"POWER_BUDGET"};
    }

    std::vector<std::function<std::string(double)> > NodePowerGovernorAgent::trace_formats(void) const
    {
        return {string_format_double};
    }

    void NodePowerGovernorAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("NodePowerGovernorAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget;
    }

    void NodePowerGovernorAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("NodePowerGovernorAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("CPU_POWER_LIMIT_CONTROL", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_POWER]);
    }

    std::string NodePowerGovernorAgent::plugin_name(void)
    {
        return "node_power_governor";
    }

    std::unique_ptr<Agent> NodePowerGovernorAgent::make_plugin(void)
    {
        return geopm::make_unique<NodePowerGovernorAgent>();
    }

    std::vector<std::string> NodePowerGovernorAgent::policy_names(void)
    {
        return {"NODE_POWER_LIMIT"};
    }

    std::vector<std::string> NodePowerGovernorAgent::sample_names(void)
    {
        return {"POWER", "IS_CONVERGED", "POWER_AVERAGE_ENFORCED"};
    }
}
