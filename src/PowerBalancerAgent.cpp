/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PowerBalancerAgent.hpp"

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <limits>

#include "PowerBalancer.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "PlatformIOProf.hpp"
#include "SampleAggregator.hpp"
#include "config.h"

namespace geopm
{
    static int calc_num_node(const std::vector<int> &fan_in)
    {
        int num_node = 1;
        for (auto fi : fan_in) {
            num_node *= fi;
        }
        return num_node;
    }

    PowerBalancerAgent::Role::Role(int num_node)
        : M_STEP_IMP({
                std::make_shared<SendDownLimitStep>(),
                std::make_shared<MeasureRuntimeStep>(),
                std::make_shared<ReduceLimitStep>()
            })
        , m_policy(M_NUM_POLICY, NAN)
        , m_step_count(-1)
        , M_NUM_NODE(num_node)
    {
#ifdef GEOPM_DEBUG
        if (M_STEP_IMP.size() != M_NUM_STEP) {
            throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): invalid M_STEP_IMP size",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

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

    const PowerBalancerAgent::Step& PowerBalancerAgent::Role::step_imp()
    {
        return *M_STEP_IMP[step()];
    }

    PowerBalancerAgent::LeafRole::LeafRole(PlatformIO &platform_io,
                                           const PlatformTopo &platform_topo,
                                           std::shared_ptr<SampleAggregator> sample_agg,
                                           std::vector<std::shared_ptr<PowerBalancer> > power_balancer,
                                           double min_power,
                                           double max_power,
                                           double time_window,
                                           bool is_single_node,
                                           int num_node)
        : Role(num_node)
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_sample_agg(sample_agg)
        , m_num_domain(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , m_count_pio_idx(m_num_domain, -1)
        , m_time_agg_idx(m_num_domain, -1)
        , m_network_agg_idx(m_num_domain, -1)
        , m_ignore_agg_idx(m_num_domain, -1)
        , m_power_balancer(power_balancer)
        , M_STABILITY_FACTOR(3.0)
        , m_package(m_num_domain, m_package_s {0, 0.0, NAN, 0.0, 0.0, false, true, -1})
        , M_MIN_PKG_POWER_SETTING(min_power)
        , M_MAX_PKG_POWER_SETTING(max_power)
        , m_is_single_node(is_single_node)
        , m_is_first_policy(true)
    {
        if (m_power_balancer.empty()) {
            double ctl_latency = M_STABILITY_FACTOR * time_window;
            for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
                m_power_balancer.push_back(PowerBalancer::make_unique(ctl_latency));
            }
        }
        init_platform_io();
        are_steps_complete(true);
    }

    PowerBalancerAgent::LeafRole::~LeafRole() = default;

    void PowerBalancerAgent::LeafRole::init_platform_io(void)
    {
        // Setup signals and controls
        for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
            m_count_pio_idx[pkg_idx] =
                m_platform_io.push_signal("EPOCH_COUNT",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_time_agg_idx[pkg_idx] =
                m_sample_agg->push_signal("TIME",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_network_agg_idx[pkg_idx] =
                m_sample_agg->push_signal("TIME_HINT_NETWORK",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_ignore_agg_idx[pkg_idx] =
                m_sample_agg->push_signal("TIME_HINT_IGNORE",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_package[pkg_idx].pio_power_idx =
                m_platform_io.push_control("CPU_POWER_LIMIT_CONTROL",
                                           GEOPM_DOMAIN_PACKAGE, pkg_idx);
        }
    }

   void PowerBalancerAgent::LeafRole::are_steps_complete(bool is_complete)
   {
       for (auto &pkg : m_package) {
           pkg.is_step_complete = is_complete;
       }
   }

   bool PowerBalancerAgent::LeafRole::are_steps_complete(void)
   {
       return std::all_of(m_package.cbegin(), m_package.cend(),
           [](const m_package_s &pkg) {
               return pkg.is_step_complete;
           });
   }

    bool PowerBalancerAgent::LeafRole::adjust_platform(const std::vector<double> &in_policy)
    {
        if (m_is_single_node) {
            if (m_is_first_policy) {
                m_policy = in_policy;
                m_is_first_policy = false;
            }
            else if (are_steps_complete()) {
                std::vector<double> sample(M_NUM_SAMPLE, 0.0);
                sample[M_SAMPLE_STEP_COUNT] = m_policy[M_POLICY_STEP_COUNT];
                sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = m_policy[M_POLICY_MAX_EPOCH_RUNTIME];
                sample[M_SAMPLE_SUM_POWER_SLACK] = m_policy[M_POLICY_POWER_SLACK];
                sample[M_SAMPLE_MIN_POWER_HEADROOM] = m_policy[M_POLICY_POWER_SLACK];
                step_imp().update_policy(*this, sample);
                m_policy[M_POLICY_STEP_COUNT] += 1;
            }
        }
        else {
            m_policy = in_policy;
        }
        if (m_policy[M_POLICY_CPU_POWER_LIMIT] != 0.0) {
            // New power cap from resource manager, reset algorithm.
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            double pkg_limit = m_policy[M_POLICY_CPU_POWER_LIMIT] / m_num_domain;
            for (auto &balancer : m_power_balancer) {
                balancer->power_cap(pkg_limit);
            }
            are_steps_complete(true);
        }
        else if (m_policy[M_POLICY_STEP_COUNT] != m_step_count) {
            // Advance a step
            ++m_step_count;
            are_steps_complete(false);
            if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::adjust_platform(): The policy step is out of sync "
                                "with the agent step or first policy received had a zero power cap.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            step_imp().enter_step(*this, m_policy);
        }

        bool result = false;
        for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
            auto &balancer = m_power_balancer[pkg_idx];
            auto &package = m_package[pkg_idx];
            // Request the power limit from the balancer
            double request_limit = balancer->power_limit();
            if (!std::isnan(request_limit) && request_limit != 0.0) {
                if (request_limit < M_MIN_PKG_POWER_SETTING) {
                    package.is_out_of_bounds = true;
                    request_limit = M_MIN_PKG_POWER_SETTING;
                }
                if (request_limit != package.actual_limit) {
                    m_platform_io.adjust(package.pio_power_idx, request_limit);
                    balancer->power_limit_adjusted(request_limit);
                    package.actual_limit = request_limit;
                    result = true;
                }
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
        m_sample_agg->update();
        double runtime = 0.0;
        double power_slack = 0.0;
        double power_headroom = 0;
        for (auto &pkg : m_package) {
            runtime = std::max(runtime, pkg.runtime);
            power_slack += pkg.power_slack;
            power_headroom += pkg.power_headroom;
        }
        out_sample[M_SAMPLE_STEP_COUNT] = m_step_count;
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = power_slack;
        out_sample[M_SAMPLE_MIN_POWER_HEADROOM] = power_headroom;

        bool result = are_steps_complete();
        if (m_is_single_node && result) {
            m_policy.at(M_POLICY_MAX_EPOCH_RUNTIME) = runtime;
            m_policy.at(M_POLICY_POWER_SLACK) = std::min(power_slack,
                                                         power_headroom);
        }
        return result;
    }

    void PowerBalancerAgent::LeafRole::trace_values(std::vector<double> &values)
    {
        // The values set include everything sampled from the
        // platform, latest policy values, and actual power limit
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) +
                            "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_POLICY_CPU_POWER_LIMIT] =
            m_policy.at(M_POLICY_CPU_POWER_LIMIT);
        values[M_TRACE_SAMPLE_POLICY_STEP_COUNT] =
            m_policy.at(M_POLICY_STEP_COUNT);
        values[M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME] =
            m_policy.at(M_POLICY_MAX_EPOCH_RUNTIME);
        values[M_TRACE_SAMPLE_POLICY_POWER_SLACK] =
            m_policy.at(M_POLICY_POWER_SLACK);
        double actual_limit = 0.0;
        for (auto &pkg : m_package) {
            actual_limit += pkg.actual_limit;
        }
        values[M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT] =
            actual_limit;
    }

    PowerBalancerAgent::TreeRole::TreeRole(int level, const std::vector<int> &fan_in)
        : Role(calc_num_node(fan_in))
        , M_AGG_FUNC({
              Agg::min, // M_SAMPLE_STEP_COUNT
              Agg::max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              Agg::sum, // M_SAMPLE_SUM_POWER_SLACK
              Agg::min, // M_SAMPLE_MIN_POWER_HEADROOM
          })
        , M_NUM_CHILDREN(fan_in[level - 1])
        , m_is_step_complete(true)
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
        Agent::aggregate_sample(in_sample, M_AGG_FUNC, out_sample);
        if (!m_is_step_complete && out_sample[M_SAMPLE_STEP_COUNT] == m_step_count) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            m_is_step_complete = true;
            if (out_sample[M_SAMPLE_STEP_COUNT] != m_step_count) {
                throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) +
                                "(): sample received has true for step complete field, " +
                                "but the step_count does not match the agent's current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return result;
    }

    PowerBalancerAgent::RootRole::RootRole(int level, const std::vector<int> &fan_in,
                                           double min_power, double max_power)
        : TreeRole(level, fan_in)
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
            if (m_step_count != m_policy.at(M_POLICY_STEP_COUNT)) {
                throw Exception("PowerBalancerAgent::RootRole::" + std::string(__func__) +
                                "(): sample passed does not match current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            step_imp().update_policy(*this, out_sample);
            m_policy.at(M_POLICY_STEP_COUNT) = m_step_count + 1;
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
        if (in_policy[M_POLICY_CPU_POWER_LIMIT] != m_root_cap) {
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_CPU_POWER_LIMIT] = in_policy[M_POLICY_CPU_POWER_LIMIT];
            m_policy[M_POLICY_STEP_COUNT] = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
            m_policy[M_POLICY_POWER_SLACK] = 0.0;
            m_root_cap = in_policy[M_POLICY_CPU_POWER_LIMIT];
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

    void PowerBalancerAgent::SendDownLimitStep::update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const
    {
        role.m_policy.at(PowerBalancerAgent::M_POLICY_CPU_POWER_LIMIT) = 0.0;
    }

    void PowerBalancerAgent::SendDownLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        double slack_power = in_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK];
        // Find the package with the least headroom at the current power limit
        double min_headroom = std::numeric_limits<double>::max();
        for (auto &balancer : role.m_power_balancer) {
            double headroom = role.M_MAX_PKG_POWER_SETTING -
                              balancer->power_limit();
            min_headroom = std::min(min_headroom, headroom);
        }
        double even_slack = slack_power / role.m_num_domain;
        if (even_slack < min_headroom) {
            slack_power = 0;
        }
        else {
            even_slack = min_headroom;
            slack_power -= even_slack * role.m_num_domain;
        }
        double total_headroom = 0.0;
        for (auto &balancer : role.m_power_balancer) {
            double headroom = role.M_MAX_PKG_POWER_SETTING -
                              (balancer->power_limit() + even_slack);
            total_headroom += headroom;
        }
        for (auto &balancer : role.m_power_balancer) {
            double headroom = role.M_MAX_PKG_POWER_SETTING -
                              (balancer->power_limit() + even_slack);
            double factor = 1.0;
            if (total_headroom != 0.0) {
                factor = headroom / total_headroom;
            }
            double cap = balancer->power_limit() + even_slack + factor * slack_power;
            balancer->power_cap(cap);
        }
        role.are_steps_complete(true);
    }

    void PowerBalancerAgent::SendDownLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const
    {
        role.m_policy.at(PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME) = sample[PowerBalancerAgent::M_SAMPLE_MAX_EPOCH_RUNTIME];
    }

    void PowerBalancerAgent::MeasureRuntimeStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        for (int pkg_idx = 0; pkg_idx < role.m_num_domain; ++pkg_idx) {
            auto &package = role.m_package[pkg_idx];
            int epoch_count = role.m_platform_io.sample(role.m_count_pio_idx[pkg_idx]);
            if (epoch_count > 1 &&
                epoch_count != package.last_epoch_count &&
                !package.is_step_complete) {
                /// We wish to measure runtime that is a function of node
                /// local optimizations only, and therefore uncorrelated
                /// between compute nodes.
                double total = role.m_sample_agg->sample_epoch_last(role.m_time_agg_idx[pkg_idx]);
                double network = role.m_sample_agg->sample_epoch_last(role.m_network_agg_idx[pkg_idx]);
                double ignore = role.m_sample_agg->sample_epoch_last(role.m_ignore_agg_idx[pkg_idx]);
                double balanced_epoch_runtime =  total - network - ignore;
                auto &balancer = role.m_power_balancer[pkg_idx];
                package.is_step_complete = balancer->is_runtime_stable(balanced_epoch_runtime);
                if (!package.is_step_complete) {
                    package.runtime = balancer->runtime_sample();
                }
            }
            package.last_epoch_count = epoch_count;
        }
    }

    void PowerBalancerAgent::ReduceLimitStep::update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const
    {
        double slack = sample[PowerBalancerAgent::M_SAMPLE_SUM_POWER_SLACK] / role.M_NUM_NODE;
        double head = sample[PowerBalancerAgent::M_SAMPLE_MIN_POWER_HEADROOM];
        role.m_policy.at(PowerBalancerAgent::M_POLICY_POWER_SLACK) = std::min(slack, head);
    }

    void PowerBalancerAgent::ReduceLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        double target = in_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME];
        for (auto balancer : role.m_power_balancer) {
            balancer->target_runtime(target);
        }
    }

    void PowerBalancerAgent::ReduceLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        for (int pkg_idx = 0; pkg_idx != role.m_num_domain; ++pkg_idx) {
            int epoch_count = role.m_platform_io.sample(role.m_count_pio_idx[pkg_idx]);
            // If all of the ranks have observed a new epoch then update
            // the power_balancer.
            auto &package = role.m_package[pkg_idx];
            auto &balancer = role.m_power_balancer[pkg_idx];
            if (epoch_count > 1 &&
                epoch_count != package.last_epoch_count &&
                !package.is_step_complete) {
                /// We wish to measure runtime that is a function of
                /// node local optimizations only, and therefore
                /// uncorrelated between compute nodes.
                double total = role.m_sample_agg->sample_epoch_last(role.m_time_agg_idx[pkg_idx]);
                double network = role.m_sample_agg->sample_epoch_last(role.m_network_agg_idx[pkg_idx]);
                double ignore = role.m_sample_agg->sample_epoch_last(role.m_ignore_agg_idx[pkg_idx]);
                double balanced_epoch_runtime =  total - network - ignore;
                package.is_step_complete = package.is_out_of_bounds ||
                                           balancer->is_target_met(balanced_epoch_runtime);
                package.power_slack = balancer->power_slack();
                package.is_out_of_bounds = false;
                package.power_headroom = role.M_MAX_PKG_POWER_SETTING - balancer->power_limit();
            }
            package.last_epoch_count = epoch_count;
        }
    }

    PowerBalancerAgent::PowerBalancerAgent()
        : PowerBalancerAgent(PlatformIOProf::platform_io(),
                             platform_topo(),
                             SampleAggregator::make_unique(),
                             {},
                             PlatformIOProf::platform_io().read_signal("CPU_POWER_MIN_AVAIL", GEOPM_DOMAIN_PACKAGE, 0),
                             PlatformIOProf::platform_io().read_signal("CPU_POWER_MAX_AVAIL", GEOPM_DOMAIN_PACKAGE, 0))
    {

    }

    PowerBalancerAgent::PowerBalancerAgent(PlatformIO &platform_io,
                                           const PlatformTopo &platform_topo,
                                           std::shared_ptr<SampleAggregator> sample_agg,
                                           std::vector<std::shared_ptr<PowerBalancer> > power_balancer,
                                           double min_power,
                                           double max_power)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_sample_agg(sample_agg)
        , m_role(nullptr)
        , m_power_balancer(power_balancer)
        , m_last_wait(time_zero())
        , M_WAIT_SEC(0.005)
        , m_power_tdp(NAN)
        , m_do_send_sample(false)
        , m_do_send_policy(false)
        , m_do_write_batch(false)
        , M_MIN_PKG_POWER_SETTING(min_power)
        , M_MAX_PKG_POWER_SETTING(max_power)
        , M_TIME_WINDOW(0.015)
    {
        geopm_time(&m_last_wait);
        m_power_tdp = m_platform_io.read_signal("CPU_POWER_LIMIT_DEFAULT", GEOPM_DOMAIN_BOARD, 0);
    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        bool is_tree_root = (level == (int)fan_in.size());
        if (level == 0) {
            m_role = std::make_shared<LeafRole>(m_platform_io,
                                                m_platform_topo,
                                                m_sample_agg,
                                                m_power_balancer,
                                                M_MIN_PKG_POWER_SETTING,
                                                M_MAX_PKG_POWER_SETTING,
                                                M_TIME_WINDOW,
                                                is_tree_root,
                                                calc_num_node(fan_in));
            m_platform_io.write_control("CPU_POWER_TIME_WINDOW",
                                        GEOPM_DOMAIN_BOARD, 0, M_TIME_WINDOW);
        }
        else if (is_tree_root) {
            m_role = std::make_shared<RootRole>(level,
                                                fan_in,
                                                M_MIN_PKG_POWER_SETTING,
                                                M_MAX_PKG_POWER_SETTING);
        }
        else {
            m_role = std::make_shared<TreeRole>(level, fan_in);
        }
    }

    void PowerBalancerAgent::split_policy(const std::vector<double> &in_policy,
                                          std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_do_send_policy =  m_role->descend(in_policy, out_policy);
    }

    bool PowerBalancerAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void PowerBalancerAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                              std::vector<double> &out_sample)
    {
        m_do_send_sample = m_role->ascend(in_sample, out_sample);
    }

    bool PowerBalancerAgent::do_send_sample(void) const
    {
        return m_do_send_sample;
    }

    void PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_do_write_batch = m_role->adjust_platform(in_policy);
    }

    bool PowerBalancerAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    void PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        m_do_send_sample = m_role->sample_platform(out_sample);
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

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_host(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerBalancerAgent::trace_names(void) const
    {
        return {"POLICY_CPU_POWER_LIMIT", // M_TRACE_SAMPLE_POLICY_CPU_POWER_LIMIT
                "POLICY_STEP_COUNT",        // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                "POLICY_MAX_EPOCH_RUNTIME", // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                "POLICY_POWER_SLACK",       // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                "ENFORCED_POWER_LIMIT",     // M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT
               };
    }

    std::vector<std::function<std::string(double)> > PowerBalancerAgent::trace_formats(void) const
    {
        return {string_format_double,         // M_TRACE_SAMPLE_POLICY_CPU_POWER_LIMIT
                format_step_count,            // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                string_format_double,         // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                string_format_double,         // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                string_format_double,         // M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT
               };
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
        m_role->trace_values(values);
    }

    void PowerBalancerAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("CPU_POWER_LIMIT_CONTROL", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_CPU_POWER_LIMIT]);
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
        return {"CPU_POWER_LIMIT",
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

    std::string PowerBalancerAgent::format_step_count(double step)
    {
        int64_t step_count = step;
        int64_t step_type = step_count % M_NUM_STEP;
        int64_t loop_count = step_count / M_NUM_STEP;
        std::string result(std::to_string(loop_count));
        switch (step_type) {
            case M_STEP_SEND_DOWN_LIMIT:
                result += "-STEP_SEND_DOWN_LIMIT";
                break;
            case M_STEP_MEASURE_RUNTIME:
                result += "-STEP_MEASURE_RUNTIME";
                break;
            case M_STEP_REDUCE_LIMIT:
                result += "-STEP_REDUCE_LIMIT";
                break;
            default:
                break;
        }
        return result;
    }

    void PowerBalancerAgent::validate_policy(std::vector<double> &policy) const
    {
        // If NAN, use default
        if (std::isnan(policy[M_POLICY_CPU_POWER_LIMIT])) {
            policy[M_POLICY_CPU_POWER_LIMIT] = m_power_tdp;
        }
        if (std::isnan(policy[M_POLICY_STEP_COUNT])) {
            policy[M_POLICY_STEP_COUNT] = 0.0;
        }
        if (std::isnan(policy[M_POLICY_MAX_EPOCH_RUNTIME])) {
            policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
        }
        if (std::isnan(policy[M_POLICY_POWER_SLACK])) {
            policy[M_POLICY_POWER_SLACK] = 0.0;
        }

        // Clamp to min or max; note that 0.0 is a valid power limit
        // when the step is not SEND_DOWN_LIMIT
        if (policy[M_POLICY_CPU_POWER_LIMIT] != 0) {
            if (policy[M_POLICY_CPU_POWER_LIMIT] < M_MIN_PKG_POWER_SETTING) {
                policy[M_POLICY_CPU_POWER_LIMIT] = M_MIN_PKG_POWER_SETTING;
            }
            else if (policy[M_POLICY_CPU_POWER_LIMIT] > M_MAX_PKG_POWER_SETTING) {
                policy[M_POLICY_CPU_POWER_LIMIT] = M_MAX_PKG_POWER_SETTING;
            }
        }

        // Policy of all zero is not valid
        if (std::all_of(policy.begin(), policy.end(),
                        [] (double x) { return x == 0.0; })) {
            throw Exception("PowerBalancerAgent: invalid policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
}
