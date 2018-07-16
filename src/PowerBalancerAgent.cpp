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

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "PowerGovernor.hpp"
#include "PowerBalancerAgent.hpp"
#include "PowerBalancer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerBalancerAgent::PowerBalancerAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_level(-1)
        , m_power_gov(nullptr)
        , m_power_balancer(nullptr)
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_agg_func {
              IPlatformIO::agg_min, // M_SAMPLE_STEP_COUNT
              IPlatformIO::agg_max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              IPlatformIO::agg_sum, // M_SAMPLE_SUM_POWER_SLACK
          }
        , m_num_children(0)
        , m_is_tree_root(false)
        , m_last_epoch_count(0)
        , m_step_count(-1)
        , m_is_step_complete(false)
        , m_root_cap(NAN)
        , m_runtime(0.0)
        , m_power_slack(0.0)
        , m_last_wait{{0,0}}
        , M_WAIT_SEC(0.005)
        , m_policy(M_NUM_POLICY, NAN)
        , m_num_node(0)
    {
#ifdef GEOPM_DEBUG
        if (m_agg_func.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent(): aggregation function vector is not the size of the policy vector",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        geopm_time(&m_last_wait);
    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        m_level = level;
        m_is_tree_root = (level == (int)fan_in.size());
        if (m_is_tree_root) {
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
        }
        else {
            m_is_step_complete = true;
        }
        m_num_node = 1.0;
        if (m_level == 0) {
            // Only do this at the leaf level.
            if (nullptr == m_power_gov) {
                m_power_gov = geopm::make_unique<PowerGovernor> (m_platform_io, m_platform_topo);
            }
            init_platform_io();
            m_power_balancer = geopm::make_unique<PowerBalancer>();
            m_num_children = 1;
        }
        else {
            m_num_children = fan_in[level - 1];
            for (auto fi : fan_in) {
                m_num_node *= fi;
            }
        }
    }

    void PowerBalancerAgent::init_platform_io(void)
    {
        m_power_gov->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT] = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
    }

    int PowerBalancerAgent::step(size_t step_count)
    {
        return (step_count % M_NUM_STEP);
    }

    int PowerBalancerAgent::step(void)
    {
        return step(m_step_count);
    }

    bool PowerBalancerAgent::descend(const std::vector<double> &policy_in, std::vector<std::vector<double> > &policy_out)
    {
#ifdef GEOPM_DEBUG
        if (policy_in.size() != M_NUM_POLICY ||
            policy_out.size() != (size_t)m_num_children) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_is_tree_root) {
            if (policy_in[M_POLICY_POWER_CAP] != m_root_cap) {
                m_step_count = M_STEP_SEND_DOWN_LIMIT;
                m_policy[M_POLICY_STEP_COUNT] = M_STEP_SEND_DOWN_LIMIT;
                m_policy[M_POLICY_POWER_CAP] = policy_in[M_POLICY_POWER_CAP];
                m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
                m_policy[M_POLICY_POWER_SLACK] = 0.0;
                m_root_cap = policy_in[M_POLICY_POWER_CAP];
                result = true;
            }
            else if (m_step_count + 1 == m_policy[M_POLICY_STEP_COUNT]) {
                ++m_step_count;
                m_is_step_complete = false;
                result = true;
            }
            else if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::descend(): updated policy is out of sync with current step",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (result) {
                for (auto &po : policy_out) {
                    po = m_policy;
                }
            }
        }
        else if (m_is_step_complete &&
                 policy_in[M_POLICY_STEP_COUNT] != m_step_count) {
            if (policy_in[M_POLICY_STEP_COUNT] == M_STEP_SEND_DOWN_LIMIT) {
                m_step_count = M_STEP_SEND_DOWN_LIMIT;
            }
            else if (policy_in[M_POLICY_STEP_COUNT] == m_step_count + 1) {
                ++m_step_count;
            }
            else {
                throw Exception("PowerBalancerAgent::descend(): polilcy is out of sync with agent step.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_is_step_complete = false;
            // Copy the input policy directly into each child's
            // policy.
            for (auto &po : policy_out) {
                po = policy_in;
            }
            m_policy = policy_in;
            result = true;
        }
        return result;
    }

    bool PowerBalancerAgent::ascend(const std::vector<std::vector<double> > &sample_in, std::vector<double> &sample_out)
    {
#ifdef GEOPM_DEBUG
        if (sample_in.size() != (size_t)m_num_children ||
            sample_out.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::ascend(): sample vectors not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        aggregate_sample(sample_in, m_agg_func, sample_out);
        if (!m_is_step_complete && sample_out[M_SAMPLE_STEP_COUNT] == m_step_count) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            m_is_step_complete = true;
            if (sample_out[M_SAMPLE_STEP_COUNT] != m_step_count) {
                throw Exception("PowerBalancerAgent::ascend(): sample recieved has true for step complete field, "
                                "but the step_count does not match the agent's current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (m_is_tree_root) {
                update_policy(sample_out);
            }
        }
        return result;
    }

    void PowerBalancerAgent::update_policy(const std::vector<double> &sample)
    {
        if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
            throw Exception("PowerBalancerAgent::update_policy(): sample passed does not match current step_count.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        switch (step()) {
            case M_STEP_SEND_DOWN_LIMIT:
                m_policy[M_POLICY_POWER_CAP] = 0.0;
                break;
            case M_STEP_MEASURE_RUNTIME:
                m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = sample[M_SAMPLE_MAX_EPOCH_RUNTIME];
                break;
            case M_STEP_REDUCE_LIMIT:
                m_policy[M_POLICY_POWER_SLACK] = sample[M_SAMPLE_SUM_POWER_SLACK] / m_num_node;
                break;
            default:
                break;
        }
        m_policy[M_POLICY_STEP_COUNT] = m_step_count + 1;
    }

    bool PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vector incorrectly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (in_policy[M_POLICY_POWER_CAP] != 0.0) {
            // New power cap from resource manager, reset
            // algorithm.
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            m_power_balancer->power_cap(in_policy[M_POLICY_POWER_CAP]);
            m_is_step_complete = true;
        }
        else if (in_policy[M_POLICY_STEP_COUNT] != m_step_count) {
            // Advance a step
            ++m_step_count;
            m_is_step_complete = false;
            if (m_step_count != in_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::adjust_platform(): The policy step is out of sync "
                                "with the agent step or first policy received had a zero power cap.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            // Record message data in the PowerBalancer object.
            switch (step()) {
                case M_STEP_REDUCE_LIMIT:
                    m_power_balancer->target_runtime(in_policy[M_POLICY_MAX_EPOCH_RUNTIME]);
                    break;
                case M_STEP_SEND_DOWN_LIMIT:
                    m_power_balancer->power_cap(m_power_balancer->power_limit() + in_policy[M_POLICY_POWER_SLACK]);
                    m_is_step_complete = true;
                    break;
                default:
                    break;
            }
        }

        double actual_limit = 0.0;
        bool result = false;
        // Request the power limit from the balancer
        double request_limit = m_power_balancer->power_limit();
        if (!std::isnan(request_limit) && request_limit != 0.0) {
            result = m_power_gov->adjust_platform(request_limit, actual_limit);
            if (result && actual_limit != request_limit) {
                if (step() == M_STEP_REDUCE_LIMIT) {
                    m_power_balancer->achieved_limit(actual_limit);
                }
                // Warn if this is a new power cap and it is too low.
                else if (in_policy[M_POLICY_POWER_CAP] != 0.0) {
                    std::cerr << "Warning: <geopm> PowerBalancerAgent: per node power cap of "
                              << in_policy[M_POLICY_POWER_CAP] << " Watts could not be maintained (request=" << actual_limit << ");" << std::endl;
                }
            }
        }
        return result;
    }

    bool PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        int epoch_count = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT]);
        // If all of the ranks have observed a new epoch then update
        // the power_balancer.
        if (epoch_count != m_last_epoch_count &&
            !m_is_step_complete) {
            double epoch_runtime = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME]);
            switch (step()) {
                case M_STEP_MEASURE_RUNTIME:
                    m_is_step_complete = m_power_balancer->is_runtime_stable(epoch_runtime);
                    m_runtime = m_power_balancer->runtime_sample();
                    break;
                case M_STEP_REDUCE_LIMIT:
                    m_is_step_complete = m_power_balancer->is_target_met(epoch_runtime);
                    m_power_slack = m_power_balancer->power_slack();
                    break;
                default:
                    break;
            }
            m_last_epoch_count = epoch_count;
        }
        m_power_gov->sample_platform();
        out_sample[M_SAMPLE_STEP_COUNT] = m_step_count;
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = m_runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = m_power_slack;
        return m_is_step_complete;
    }


    void PowerBalancerAgent::wait(void)    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_node(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerBalancerAgent::trace_names(void) const
    {
        return {"epoch_runtime",
                "power_limit"};
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (m_level != 0) {
            throw Exception("PowerBalancerAgent::trace_values(): called on non-leaf agent.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_power_balancer->runtime_sample();
        values[M_TRACE_SAMPLE_POWER_LIMIT] = m_power_balancer->power_limit();
    }

    std::string PowerBalancerAgent::plugin_name(void)
    {
        return "power_balancer";
    }

    std::unique_ptr<Agent> PowerBalancerAgent::make_plugin(void)
    {
        return geopm::make_unique<PowerBalancerAgent>();
    }

    std::vector<std::string> PowerBalancerAgent::policy_names(void)
    {
        return {"STEP_COUNT",
                "POWER_CAP",
                "MAX_EPOCH_RUNTIME",
                "POWER_SLACK"};
    }

    std::vector<std::string> PowerBalancerAgent::sample_names(void)
    {
        return {"STEP_COUNT",
                "MAX_EPOCH_RUNTIME",
                "SUM_POWER_SLACK"};
    }
}
