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
#include <limits>

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
    PowerBalancerAgent::Role::Role()
        : M_STEP_IMP({
                std::make_shared<SendDownLimitStep>(),
                std::make_shared<MeasureRuntimeStep>(),
                std::make_shared<ReduceLimitStep>()
            })
        , m_policy(M_NUM_POLICY, NAN)
        , m_step_count(std::numeric_limits<std::size_t>::max())
        , m_is_step_complete(true)
    {
#ifdef GEOPM_DEBUG
        if (M_STEP_IMP.size() != M_NUM_STEP) {
            throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): invalid M_STEP_IMP size",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    PowerBalancerAgent::Role::~Role() = default;

    bool PowerBalancerAgent::Role::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    std::vector<std::string> PowerBalancerAgent::Role::trace_names(void) const
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return {};
    }

    void PowerBalancerAgent::Role::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    size_t PowerBalancerAgent::Role::reset()
    {
        m_step_count = M_STEP_SEND_DOWN_LIMIT;
        return m_step_count;
    }

    size_t PowerBalancerAgent::Role::step_count() const
    {
        return m_step_count;
    }

    void PowerBalancerAgent::Role::inc_step_count()
    {
        m_step_count++;
    }

    bool PowerBalancerAgent::Role::is_step_complete() const
    {
        return m_is_step_complete;
    }

    void PowerBalancerAgent::Role::is_step_complete(bool is_complete)
    {
        m_is_step_complete = is_complete;
    }

    size_t PowerBalancerAgent::Role::step(void) const
    {
        return m_step_count % M_NUM_STEP;
    }

    void PowerBalancerAgent::Role::step(size_t step)
    {
        if (m_is_step_complete && m_step_count != step) {
            if (M_STEP_SEND_DOWN_LIMIT == step) {
                reset();
            }
            else if (m_step_count + 1 == step) {
                inc_step_count();
            }
            else {
                throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): step is out of sync with current step",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            is_step_complete(false);
        }
        else if (m_step_count + 1 == step) {
            inc_step_count();
            is_step_complete(false);
        }
        else if (m_step_count != step) {
            throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): step is out of sync with current step",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    const PowerBalancerAgent::IStep& PowerBalancerAgent::Role::step_imp()
    {
        return *M_STEP_IMP[step()];
    }

    PowerBalancerAgent::LeafRole::LeafRole(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                                           std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer)
        : Role()
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_epoch_count(0)
        , m_runtime(0.0)
        , m_actual_limit(NAN)
        , m_power_slack(0.0)
    {
        if (nullptr == m_power_governor) {
            m_power_governor = geopm::make_unique<PowerGovernor>(m_platform_io, m_platform_topo);
        }
        if (nullptr == m_power_balancer) {
            m_power_balancer = geopm::make_unique<PowerBalancer>();
        }
        init_platform_io();
    }

    PowerBalancerAgent::LeafRole::~LeafRole() = default;

    void PowerBalancerAgent::LeafRole::init_platform_io(void)
    {
        m_power_governor->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT] = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
    }

    bool PowerBalancerAgent::LeafRole::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): policy vector incorrectly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_policy = in_policy;
        if (in_policy[M_POLICY_POWER_CAP] != 0.0) {
            // New power cap from resource manager, reset
            // algorithm.
            reset();
            m_power_balancer->power_cap(in_policy[M_POLICY_POWER_CAP]);
            is_step_complete(true);
        }
        else {
            // Advance a step
            step(in_policy[M_POLICY_STEP_COUNT]);
            step_imp().pre_adjust(*this, in_policy);
        }

        bool result = false;
        // Request the power limit from the balancer
        double request_limit = m_power_balancer->power_limit();
        if (!std::isnan(request_limit) && request_limit != 0.0) {
            result = m_power_governor->adjust_platform(request_limit, m_actual_limit);
            if (result && m_actual_limit != request_limit) {
                step_imp().post_adjust(*this, in_policy[M_POLICY_POWER_CAP], m_actual_limit);
            }
        }
        return result;
    }

    bool PowerBalancerAgent::LeafRole::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        int epoch_count = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT]);
        // If all of the ranks have observed a new epoch then update
        // the power_balancer.
        if (epoch_count != m_last_epoch_count &&
            !is_step_complete()) {
            double epoch_runtime = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME]);
            step_imp().post_sample(*this, epoch_runtime);
            m_last_epoch_count = epoch_count;
        }
        m_power_governor->sample_platform();
        out_sample[M_SAMPLE_STEP_COUNT] = step_count();
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = m_runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = m_power_slack;
        return is_step_complete();
    }

    std::vector<std::string> PowerBalancerAgent::LeafRole::trace_names(void) const
    {
        return {"epoch_runtime",            // M_TRACE_SAMPLE_EPOCH_RUNTIME
                "power_limit",              // M_TRACE_SAMPLE_POWER_LIMIT
                "policy_power_cap",         // M_TRACE_SAMPLE_POLICY_POWER_CAP
                "policy_step_count",        // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                "policy_max_epoch_runtime", // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                "policy_power_slack",       // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                "policy_power_limit"        // M_TRACE_SAMPLE_POLICY_POWER_LIMIT
               };
    }

    void PowerBalancerAgent::LeafRole::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform, latest policy values, and actual power limit
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_power_balancer->runtime_sample();
        values[M_TRACE_SAMPLE_POWER_LIMIT] = m_power_balancer->power_limit();
        values[M_TRACE_SAMPLE_POLICY_POWER_CAP] = m_policy[M_POLICY_POWER_CAP];
        values[M_TRACE_SAMPLE_POLICY_STEP_COUNT] = m_policy[M_POLICY_STEP_COUNT];
        values[M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME] = m_policy[M_POLICY_MAX_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_POLICY_POWER_SLACK] = m_policy[M_POLICY_POWER_SLACK];
        values[M_TRACE_SAMPLE_POLICY_POWER_LIMIT] = m_actual_limit;
    }

    PowerBalancerAgent::TreeRole::TreeRole(int level, const std::vector<int> &fan_in)
        : Role()
        , M_AGG_FUNC({
              IPlatformIO::agg_min, // M_SAMPLE_STEP_COUNT
              IPlatformIO::agg_max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              IPlatformIO::agg_sum, // M_SAMPLE_SUM_POWER_SLACK
          })
        , M_NUM_CHILDREN(fan_in[level - 1])
    {
#ifdef GEOPM_DEBUG
        if (M_AGG_FUNC.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): aggregation function vector is not the size of the policy vector",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    PowerBalancerAgent::TreeRole::~TreeRole() = default;

    bool PowerBalancerAgent::TreeRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY ||
            out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        step(m_policy[M_POLICY_STEP_COUNT]);
        for (auto &po : out_policy) {
            po = in_policy;
        }
        m_policy = in_policy;
        return true;
    }

    bool PowerBalancerAgent::TreeRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (in_sample.size() != (size_t)M_NUM_CHILDREN ||
            out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): sample vectors not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        aggregate_sample(in_sample, M_AGG_FUNC, out_sample);
        if (!is_step_complete() && out_sample[M_SAMPLE_STEP_COUNT] == step_count()) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            is_step_complete(true);
        }
        return result;
    }

    static int calc_num_node(const std::vector<int> &fan_in)
    {
        int num_node = 1;
        for (auto fi : fan_in) {
            num_node *= fi;
        }
        return num_node;
    }

    PowerBalancerAgent::RootRole::RootRole(int level, const std::vector<int> &fan_in)
        : TreeRole(level, fan_in)
        , M_NUM_NODE(calc_num_node(fan_in))
        , m_root_cap(NAN)
    {
        reset();
    }

    PowerBalancerAgent::RootRole::~RootRole() = default;


    bool PowerBalancerAgent::RootRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
        if (step_count() != m_policy[M_POLICY_STEP_COUNT]) {
            throw Exception("RootRole::" + std::string(__func__) + "(): sample passed does not match current step_count.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        bool result = TreeRole::ascend(in_sample, out_sample);
        result |=  step_imp().update_policy(*this, out_sample);
        m_policy[M_POLICY_STEP_COUNT] = step_count() + 1;
        return result;
    }

    bool PowerBalancerAgent::RootRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY ||
            out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (in_policy[M_POLICY_POWER_CAP] != m_root_cap) {
            m_policy[M_POLICY_POWER_CAP] = in_policy[M_POLICY_POWER_CAP];
            m_policy[M_POLICY_STEP_COUNT] = reset();
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
            m_policy[M_POLICY_POWER_SLACK] = 0.0;
            m_root_cap = in_policy[M_POLICY_POWER_CAP];
        }
        else {
            step(m_policy[M_POLICY_STEP_COUNT]);
        }
        for (auto &op : out_policy) {
            op = m_policy;
        }
        return true;
    }

    bool PowerBalancerAgent::SendDownLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_CAP] = 0.0;
        return true;
    }

    void PowerBalancerAgent::SendDownLimitStep::pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        role.m_power_balancer->power_cap(role.m_power_balancer->power_limit() + in_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK]);
        role.is_step_complete(true);
    }

    void PowerBalancerAgent::SendDownLimitStep::post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit) const
    {
        if (policy_limit != 0.0) {
            std::cerr << "Warning: <geopm> PowerBalancerAgent: per node power cap of "
                << policy_limit << " Watts could not be maintained (request=" << actual_limit << ");" << std::endl;
        }
    }

    void PowerBalancerAgent::SendDownLimitStep::post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime) const
    {
    }

    bool PowerBalancerAgent::MeasureRuntimeStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME] = sample[PowerBalancerAgent::M_SAMPLE_MAX_EPOCH_RUNTIME];
        return true;
    }

    void PowerBalancerAgent::MeasureRuntimeStep::pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit) const
    {
        if (policy_limit != 0.0) {
            std::cerr << "Warning: <geopm> PowerBalancerAgent: per node power cap of "
                << policy_limit << " Watts could not be maintained (request=" << actual_limit << ");" << std::endl;
        }
    }

    void PowerBalancerAgent::MeasureRuntimeStep::post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime) const
    {
        role.m_runtime = role.m_power_balancer->runtime_sample();
        role.is_step_complete(role.m_power_balancer->is_runtime_stable(epoch_runtime));
    }

    bool PowerBalancerAgent::ReduceLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK] = sample[PowerBalancerAgent::M_SAMPLE_SUM_POWER_SLACK] / role.M_NUM_NODE;
        return true;
    }

    void PowerBalancerAgent::ReduceLimitStep::pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        role.m_power_balancer->target_runtime(in_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME]);
    }

    void PowerBalancerAgent::ReduceLimitStep::post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit) const
    {
        role.m_power_balancer->achieved_limit(actual_limit);
    }

    void PowerBalancerAgent::ReduceLimitStep::post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime) const
    {
        role.m_power_slack = role.m_power_balancer->power_cap() - role.m_power_balancer->power_limit();
        role.is_step_complete(role.m_power_balancer->is_target_met(epoch_runtime));
    }

    PowerBalancerAgent::PowerBalancerAgent()
        : PowerBalancerAgent(platform_io(), platform_topo(), nullptr, nullptr)
    {

    }

    PowerBalancerAgent::PowerBalancerAgent(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                                           std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_role(nullptr)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_wait{{0,0}}
        , M_WAIT_SEC(0.005)
    {
        geopm_time(&m_last_wait);
    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        if (fan_in.size() == (size_t) 0) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) +
                            "(): single node job detected, user power_governor.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        bool is_tree_root;
        is_tree_root = (level == (int)fan_in.size());
        if (is_tree_root) {
            m_role = std::make_shared<RootRole>(level, fan_in);
        }
        else if (level == 0) {
            m_role = std::make_shared<LeafRole>(m_platform_io, m_platform_topo, std::move(m_power_governor), std::move(m_power_balancer));
        }
        else {
            m_role = std::make_shared<TreeRole>(level, fan_in);
        }
    }

    bool PowerBalancerAgent::descend(const std::vector<double> &policy_in, std::vector<std::vector<double> > &policy_out)
    {
        return m_role->descend(policy_in, policy_out);
    }

    bool PowerBalancerAgent::ascend(const std::vector<std::vector<double> > &sample_in, std::vector<double> &sample_out)
    {
        return m_role->ascend(sample_in, sample_out);
    }

    bool PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        return m_role->adjust_platform(in_policy);
    }

    bool PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        return m_role->sample_platform(out_sample);
    }


    void PowerBalancerAgent::wait(void) {
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
        return m_role->trace_names();
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
        m_role->trace_values(values);
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
        return {"POWER_CAP",
                "STEP_COUNT",
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
