/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include "Agg.hpp"
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
        , m_step_count(-1)
        , m_is_step_complete(false)
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

    int PowerBalancerAgent::Role::step(size_t step_count) const
    {
        return (step_count % M_NUM_STEP);
    }

    int PowerBalancerAgent::Role::step(void) const
    {
        return step(m_step_count);
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
        , m_power_max(m_platform_topo.num_domain(IPlatformTopo::M_DOMAIN_PACKAGE) *
                      m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_epoch_count(0)
        , m_runtime(0.0)
        , m_actual_limit(NAN)
        , m_power_slack(0.0)
        , m_power_headroom(0.0)
        , M_STABILITY_FACTOR(3.0)
        , m_is_out_of_bounds(false)
    {
        if (nullptr == m_power_governor) {
            m_power_governor = geopm::make_unique<PowerGovernor>(m_platform_io, m_platform_topo);
        }
        if (nullptr == m_power_balancer) {
            m_power_balancer = geopm::make_unique<PowerBalancer>(M_STABILITY_FACTOR * m_power_governor->power_package_time_window());
        }
        init_platform_io();
        m_is_step_complete = true;
    }

    PowerBalancerAgent::LeafRole::~LeafRole() = default;

    void PowerBalancerAgent::LeafRole::init_platform_io(void)
    {
        m_power_governor->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT] = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME_MPI] = m_platform_io.push_signal("EPOCH_RUNTIME_MPI", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE] = m_platform_io.push_signal("EPOCH_RUNTIME_IGNORE", IPlatformTopo::M_DOMAIN_BOARD, 0);
    }

    bool PowerBalancerAgent::LeafRole::adjust_platform(const std::vector<double> &in_policy)
    {
        m_policy = in_policy;
        if (in_policy[M_POLICY_POWER_CAP] != 0.0) {
            // New power cap from resource manager, reset
            // algorithm.
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            m_power_balancer->power_cap(in_policy[M_POLICY_POWER_CAP]);
            if (in_policy[M_POLICY_POWER_CAP] > m_power_max) {
                m_power_max = in_policy[M_POLICY_POWER_CAP];
            }
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
            step_imp().enter_step(*this, in_policy);
        }

        bool result = false;
        // Request the power limit from the balancer
        double request_limit = m_power_balancer->power_limit();
        if (!std::isnan(request_limit) && request_limit != 0.0) {
            result = m_power_governor->adjust_platform(request_limit, m_actual_limit);
            if (request_limit < m_actual_limit) {
                m_is_out_of_bounds = true;
            }
            if (result) {
                m_power_balancer->power_limit_adjusted(m_actual_limit);
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
        step_imp().sample_platform(*this);
        m_power_governor->sample_platform();
        out_sample[M_SAMPLE_STEP_COUNT] = m_step_count;
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = m_runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = m_power_slack;
        out_sample[M_SAMPLE_MIN_POWER_HEADROOM] = m_power_headroom;
        return m_is_step_complete;
    }

    std::vector<std::string> PowerBalancerAgent::LeafRole::trace_names(void) const
    {
        return {"epoch_runtime",            // M_TRACE_SAMPLE_EPOCH_RUNTIME
                "policy_power_limit",       // M_TRACE_SAMPLE_POLICY_POWER_LIMIT
                "policy_power_cap",         // M_TRACE_SAMPLE_POLICY_POWER_CAP
                "policy_step_count",        // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                "policy_max_epoch_runtime", // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                "policy_power_slack",       // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                "enforced_power_limit"      // M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT
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
        values[M_TRACE_SAMPLE_POLICY_POWER_LIMIT] = m_power_balancer->power_limit();
        values[M_TRACE_SAMPLE_POLICY_POWER_CAP] = m_policy[M_POLICY_POWER_CAP];
        values[M_TRACE_SAMPLE_POLICY_STEP_COUNT] = m_policy[M_POLICY_STEP_COUNT];
        values[M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME] = m_policy[M_POLICY_MAX_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_POLICY_POWER_SLACK] = m_policy[M_POLICY_POWER_SLACK];
        values[M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT] = m_actual_limit;
    }

    PowerBalancerAgent::TreeRole::TreeRole(int level, const std::vector<int> &fan_in)
        : Role()
        , M_AGG_FUNC({
              Agg::min, // M_SAMPLE_STEP_COUNT
              Agg::max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              Agg::sum, // M_SAMPLE_SUM_POWER_SLACK
              Agg::min, // M_SAMPLE_MIN_POWER_HEADROOM
          })
        , M_NUM_CHILDREN(fan_in[level - 1])
    {
#ifdef GEOPM_DEBUG
        if (M_AGG_FUNC.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): aggregation function vector is not the size of the policy vector",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_is_step_complete = true;
    }

    PowerBalancerAgent::TreeRole::~TreeRole() = default;

    bool PowerBalancerAgent::TreeRole::descend(const std::vector<double> &in_policy,
                                               std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_is_step_complete &&
            in_policy[M_POLICY_STEP_COUNT] != m_step_count) {
            if (in_policy[M_POLICY_STEP_COUNT] == M_STEP_SEND_DOWN_LIMIT) {
                m_step_count = M_STEP_SEND_DOWN_LIMIT;
            }
            else if (in_policy[M_POLICY_STEP_COUNT] == m_step_count + 1) {
                ++m_step_count;
            }
            else {
                throw Exception("PowerBalancerAgent::descend(): policy is out of sync with agent step.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_is_step_complete = false;
            // Copy the input policy directly into each child's
            // policy.
            for (auto &po : out_policy) {
                po = in_policy;
            }
            m_policy = in_policy;
            result = true;
        }
        return result;
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
        if (!m_is_step_complete && out_sample[M_SAMPLE_STEP_COUNT] == m_step_count) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            m_is_step_complete = true;
            if (out_sample[M_SAMPLE_STEP_COUNT] != m_step_count) {
                throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) +
                                "(): sample recieved has true for step complete field, " +
                                "but the step_count does not match the agent's current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
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

    PowerBalancerAgent::RootRole::RootRole(int level, const std::vector<int> &fan_in,
                                           double min_power, double max_power)
        : TreeRole(level, fan_in)
        , M_NUM_NODE(calc_num_node(fan_in))
        , m_root_cap(NAN)
        , M_MIN_PKG_POWER_SETTING(min_power)
        , M_MAX_PKG_POWER_SETTING(max_power)
    {
        m_step_count = M_STEP_SEND_DOWN_LIMIT;
        m_is_step_complete = false;
    }

    PowerBalancerAgent::RootRole::~RootRole() = default;

    bool PowerBalancerAgent::RootRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
        bool result = TreeRole::ascend(in_sample, out_sample);
        if (result) {
            if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::RootRole::" + std::string(__func__) +
                                "(): sample passed does not match current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            step_imp().update_policy(*this, out_sample);
            m_policy[M_POLICY_STEP_COUNT] = m_step_count + 1;
        }
        return result;
    }

    bool PowerBalancerAgent::RootRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (in_policy[M_POLICY_POWER_CAP] != m_root_cap) {
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_POWER_CAP] = in_policy[M_POLICY_POWER_CAP];
            m_policy[M_POLICY_STEP_COUNT] = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
            m_policy[M_POLICY_POWER_SLACK] = 0.0;
            m_root_cap = in_policy[M_POLICY_POWER_CAP];
            if (m_root_cap > M_MAX_PKG_POWER_SETTING ||
                m_root_cap < M_MIN_PKG_POWER_SETTING) {
                throw Exception("PowerBalancerAgent::descend(): invalid power budget: " + std::to_string(m_root_cap),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
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
            for (auto &po : out_policy) {
                po = m_policy;
            }
        }
        return result;
    }

    void PowerBalancerAgent::SendDownLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_CAP] = 0.0;
    }

    void PowerBalancerAgent::SendDownLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        role.m_power_balancer->power_cap(role.m_power_balancer->power_limit() + in_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK]);
        role.m_is_step_complete = true;
    }

    void PowerBalancerAgent::SendDownLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME] = sample[PowerBalancerAgent::M_SAMPLE_MAX_EPOCH_RUNTIME];
    }

    void PowerBalancerAgent::MeasureRuntimeStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        int epoch_count = role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_COUNT]);
        // If all of the ranks have observed a new epoch then update
        // the power_balancer.
        if (epoch_count != role.m_last_epoch_count &&
            !role.m_is_step_complete) {

            /// We wish to measure runtime that is a function of node local optimizations only, and therefore uncorrelated between compute nodes.
            double balanced_epoch_runtime = role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME]) -
                                            role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_MPI]) -
                                            role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE]);
            role.m_is_step_complete = role.m_power_balancer->is_runtime_stable(balanced_epoch_runtime);
            role.m_power_balancer->calculate_runtime_sample();
            role.m_runtime = role.m_power_balancer->runtime_sample();
            role.m_last_epoch_count = epoch_count;
        }
    }

    void PowerBalancerAgent::ReduceLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        double slack = sample[PowerBalancerAgent::M_SAMPLE_SUM_POWER_SLACK] / role.M_NUM_NODE;
        double head = sample[PowerBalancerAgent::M_SAMPLE_MIN_POWER_HEADROOM];
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK] = slack < head ? slack : head;
    }

    void PowerBalancerAgent::ReduceLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        role.m_power_balancer->target_runtime(in_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME]);
    }

    void PowerBalancerAgent::ReduceLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        int epoch_count = role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_COUNT]);
        // If all of the ranks have observed a new epoch then update
        // the power_balancer.
        if (epoch_count != role.m_last_epoch_count &&
            !role.m_is_step_complete) {

            /// We wish to measure runtime that is a function of node local optimizations only, and therefore uncorrelated between compute nodes.
            double balanced_epoch_runtime = role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME]) -
                                            role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_MPI]) -
                                            role.m_platform_io.sample(role.m_pio_idx[PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE]);
            role.m_power_balancer->calculate_runtime_sample();
            role.m_is_step_complete = role.m_is_out_of_bounds ||
                                      role.m_power_balancer->is_target_met(balanced_epoch_runtime);
            role.m_power_slack = role.m_power_balancer->power_slack();
            role.m_is_out_of_bounds = false;
            role.m_power_headroom = role.m_power_max - role.m_power_balancer->power_limit();
            role.m_last_epoch_count = epoch_count;
        }
    }

    PowerBalancerAgent::PowerBalancerAgent()
        : PowerBalancerAgent(platform_io(), platform_topo(), nullptr, nullptr)
    {

    }

    PowerBalancerAgent::PowerBalancerAgent(IPlatformIO &platform_io,
                                           IPlatformTopo &platform_topo,
                                           std::unique_ptr<IPowerGovernor> power_governor,
                                           std::unique_ptr<IPowerBalancer> power_balancer)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_role(nullptr)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_wait(GEOPM_TIME_REF)
        , M_WAIT_SEC(0.005)
        , m_power_tdp(NAN)
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
        bool is_tree_root = (level == (int)fan_in.size());
        if (is_tree_root) {
            int num_pkg = m_platform_topo.num_domain(m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT"));
            double min_power = num_pkg * m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
            double max_power = num_pkg * m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
            m_power_tdp = num_pkg * m_platform_io.read_signal("POWER_PACKAGE_TDP", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
            m_role = std::make_shared<RootRole>(level, fan_in, min_power, max_power);
        }
        else if (level == 0) {
            m_role = std::make_shared<LeafRole>(m_platform_io, m_platform_topo, std::move(m_power_governor), std::move(m_power_balancer));
        }
        else {
            m_role = std::make_shared<TreeRole>(level, fan_in);
        }
    }

    bool PowerBalancerAgent::descend(const std::vector<double> &in_policy, std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        auto policy = validate_policy(in_policy);
        return m_role->descend(policy, out_policy);
    }

    bool PowerBalancerAgent::ascend(const std::vector<std::vector<double> > &sample_in, std::vector<double> &sample_out)
    {
        return m_role->ascend(sample_in, sample_out);
    }

    bool PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        auto policy = validate_policy(in_policy);
        return m_role->adjust_platform(policy);
    }

    bool PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        return m_role->sample_platform(out_sample);
    }

    void PowerBalancerAgent::wait(void)    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
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
                "SUM_POWER_SLACK",
                "MIN_POWER_HEADROOM"};
    }

    std::vector<double> PowerBalancerAgent::validate_policy(const std::vector<double> &in_policy) const
    {
        // If NAN, use default
        std::vector<double> updated_policy = in_policy;
        if (std::isnan(in_policy[M_POLICY_POWER_CAP])) {
            updated_policy[M_POLICY_POWER_CAP] = m_power_tdp;
        }
        if (std::isnan(in_policy[M_POLICY_STEP_COUNT])) {
            updated_policy[M_POLICY_STEP_COUNT] = 0.0;
        }
        if (std::isnan(in_policy[M_POLICY_MAX_EPOCH_RUNTIME])) {
            updated_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
        }
        if (std::isnan(in_policy[M_POLICY_POWER_SLACK])) {
            updated_policy[M_POLICY_POWER_SLACK] = 0.0;
        }
        // Policy of all zero is not valid
        if (std::all_of(updated_policy.begin(), updated_policy.end(),
                        [] (double x) { return x == 0.0; })) {
            throw Exception("PowerBalancerAgent: invalid policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return updated_policy;
    }
}
