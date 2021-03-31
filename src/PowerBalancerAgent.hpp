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

#ifndef POWERBALANCERAGENT_HPP_INCLUDE
#define POWERBALANCERAGENT_HPP_INCLUDE

#include <vector>
#include <functional>

#include "geopm_time.h"
#include "Agent.hpp"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;
    class PowerBalancer;
    class PowerGovernor;
    class SampleAggregator;

    class PowerBalancerAgent : public Agent
    {
        public:
            enum m_policy_e {
                /// @brief The power cap enforced on average over all
                ///        nodes running the application.  This has
                ///        value 0.0 except in two cases.  In the
                ///        first case this is the M_SEND_DOWN_LIMIT
                ///        step at the beginning of the application
                ///        run.  This value will also be non-zero in
                ///        the case where the resource mananger has
                ///        requested a new budget for the application,
                ///        and thus, the algorithm must be restarted
                ///        at step M_SEND_DOWN_LIMIT.
                M_POLICY_POWER_PACKAGE_LIMIT_TOTAL,
                /// @brief Step that the root is providing a policy
                ///        for.  The parent has received a sample
                ///        matching this step in the last walk up the
                ///        tree, except in the case where the root
                ///        Agent has recently been updated with a new
                ///        policy; in this case the step will be
                ///        M_SEND_DOWN_LIMIT and the policy indexed by
                ///        M_POLICY_POWER_CAP will have a non-zero
                ///        value.
                M_POLICY_STEP_COUNT,
                /// @brief The largest runtime reported by any leaf
                ///        agent since the last redistribution of
                ///        power.  This will have value 0.0 until all
                ///        leaf agents have reported a runtime to the
                ///        root agent.
                M_POLICY_MAX_EPOCH_RUNTIME,
                /// @brief This value is updated in step
                ///        M_STEP_ADJUST_LIMIT to the amount that each
                ///        leaf agent should increase their power
                ///        limit by calling:
                ///            power_cap(current_limit + slack)
                ///        by before starting the algorithm over again
                ///        at step M_STEP_MEASURE_RUNTIME.  For all
                ///        other steps this value is 0.0.
                M_POLICY_POWER_SLACK,
                /// @brief Number of steps in each iteration of the
                ///        balancing algorithm.
                M_NUM_POLICY,
            };

            enum m_sample_e {
                /// @brief The the step counter that is currently in
                ///        execution.  Note that the step is equal to
                ///        the step counter modulo M_NUM_STEP and is
                ///        reset each time a new power cap is provided
                ///        by sending a policy with a non-zero
                ///        M_POLICY_POWER_CAP field.
                M_SAMPLE_STEP_COUNT,
                /// @brief Maximum expected runtime for any node
                ///        below.
                M_SAMPLE_MAX_EPOCH_RUNTIME,
                /// @brief The sum of all slack power available from
                ///        children below the agent.
                M_SAMPLE_SUM_POWER_SLACK,
                /// @brief Smallest difference between maximum power
                ///        limit and current power limit for any node
                ///        below.
                M_SAMPLE_MIN_POWER_HEADROOM,
                /// @brief Number of elements in a sample vector.
                M_NUM_SAMPLE,
            };

            enum m_trace_sample_e {
                M_TRACE_SAMPLE_POLICY_POWER_PACKAGE_LIMIT_TOTAL,
                M_TRACE_SAMPLE_POLICY_STEP_COUNT,
                M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME,
                M_TRACE_SAMPLE_POLICY_POWER_SLACK,
                M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT,
                M_TRACE_NUM_SAMPLE,
            };

            enum m_step_e {
                /// @brief On first iteration send down resource
                ///        manager average limit requested, otherwise
                ///        send down average excess power.
                M_STEP_SEND_DOWN_LIMIT = 0L,
                /// @brief Measure epoch runtime several times and
                ///        apply median filter.  Aggregate epoch
                ///        runtime up tree by applying maximum filter
                ///        to measured values.  Propagate down from
                ///        root the longest recorded runtime from any
                ///        node.
                M_STEP_MEASURE_RUNTIME,
                /// @brief Decrease power limit on all nodes (other
                ///        than the slowest) until epoch runtime
                ///        matches the slowest.  Aggregate amount
                ///        power limit was reduced in last step up the
                ///        tree with sum filter.  (Go to
                ///        M_STEP_SEND_DOWN_LIMIT next).
                M_STEP_REDUCE_LIMIT,
                /// @brief Number of steps in process.
                M_NUM_STEP,
            };

            PowerBalancerAgent(PlatformIO &platform_io,
                               const PlatformTopo &platform_topo,
                               std::shared_ptr<SampleAggregator> sample_agg,
                               std::vector<std::shared_ptr<PowerBalancer> > power_balancer,
                               double min_power,
                               double max_power);
            PowerBalancerAgent();
            virtual ~PowerBalancerAgent();
            void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
            void validate_policy(std::vector<double> &policy) const override;
            void split_policy(const std::vector<double> &in_policy,
                              std::vector<std::vector<double> > &out_policy) override;
            bool do_send_policy(void) const override;
            void aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                  std::vector<double> &out_sample) override;
            bool do_send_sample(void) const override;
            void adjust_platform(const std::vector<double> &in_policy) override;
            bool do_write_batch(void) const override;
            void sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::vector<std::pair<std::string, std::string> > report_header(void) const override;
            std::vector<std::pair<std::string, std::string> > report_host(void) const override;
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
            std::vector<std::string> trace_names(void) const override;
            std::vector<std::function<std::string(double)> > trace_formats(void) const override;
            void trace_values(std::vector<double> &values) override;
            void enforce_policy(const std::vector<double> &policy) const override;

            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
            static std::string format_step_count(double step);
        protected:
            class Step;
            class Role {
                public:
                    /// Tree role classes must implement this method,
                    /// leaf roles do not.
                    virtual bool descend(const std::vector<double> &in_policy,
                                         std::vector<std::vector<double> >&out_policy);
                    /// Tree role classes must implement this method,
                    /// leaf roles do not.
                    virtual bool ascend(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double> &out_sample);
                    /// Leaf role classes must implement this method,
                    /// tree roles do not.
                    virtual bool adjust_platform(const std::vector<double> &in_policy);
                    /// Leaf role classes must implement this method,
                    /// tree roles do not.
                    virtual bool sample_platform(std::vector<double> &out_sample);
                    /// Leaf role classes must implement this method,
                    /// tree roles do not.
                    virtual void trace_values(std::vector<double> &values);
                protected:
                    int step(size_t step_count) const;
                    int step(void) const;
                    const Step& step_imp();
                    Role(int num_node);
                    virtual ~Role() = default;
                    const std::vector<std::shared_ptr<const Step> > M_STEP_IMP;
                public:
                    std::vector<double> m_policy;
                protected:
                    int m_step_count;
                public:
                    const int M_NUM_NODE;
            };

            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<SampleAggregator> m_sample_agg;
            std::shared_ptr<Role> m_role;
            std::vector<std::shared_ptr<PowerBalancer> > m_power_balancer;
            struct geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
            double m_power_tdp;
            bool m_do_send_sample;
            bool m_do_send_policy;
            bool m_do_write_batch;
            const double M_MIN_PKG_POWER_SETTING;
            const double M_MAX_PKG_POWER_SETTING;
            const double M_TIME_WINDOW;

            class RootRole;
            class LeafRole;
            class TreeRole;

            class Step {
                public:
                    Step() = default;
                    virtual ~Step() = default;
                    virtual void update_policy(Role &role, const std::vector<double> &sample) const = 0;
                    virtual void enter_step(LeafRole &role, const std::vector<double> &in_policy) const = 0;
                    virtual void sample_platform(LeafRole &role) const = 0;
            };

            class SendDownLimitStep : public Step {
                public:
                    SendDownLimitStep() = default;
                   ~SendDownLimitStep() = default;
                   void update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const;
                   void enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const;
                   void sample_platform(PowerBalancerAgent::LeafRole &role) const;
            };

            class MeasureRuntimeStep : public Step {
                public:
                    MeasureRuntimeStep() = default;
                    ~MeasureRuntimeStep() = default;
                    void update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const;
                    void enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const;
                    void sample_platform(PowerBalancerAgent::LeafRole &role) const;
            };

            class ReduceLimitStep : public Step {
                public:
                    ReduceLimitStep() = default;
                    ~ReduceLimitStep() = default;
                    void update_policy(PowerBalancerAgent::Role &role, const std::vector<double> &sample) const;
                    void enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const;
                    void sample_platform(PowerBalancerAgent::LeafRole &role) const;
            };

            class TreeRole : public Role {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    TreeRole(int level, const std::vector<int> &fan_in);
                    virtual ~TreeRole();
                    virtual bool descend(const std::vector<double> &in_policy,
                                         std::vector<std::vector<double> >&out_policy) override;
                    virtual bool ascend(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double> &out_sample) override;
                protected:
                    const std::vector<std::function<double(const std::vector<double>&)> > M_AGG_FUNC;
                    const int M_NUM_CHILDREN;
                    bool m_is_step_complete;
            };

            class RootRole : public TreeRole {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    RootRole(int level, const std::vector<int> &fan_in, double min_power, double max_power);
                    virtual ~RootRole();
                    bool descend(const std::vector<double> &in_policy,
                                 std::vector<std::vector<double> >&out_policy) override;
                    bool ascend(const std::vector<std::vector<double> > &in_sample,
                                std::vector<double> &out_sample) override;
                private:
                    double m_root_cap;
                    const double M_MIN_PKG_POWER_SETTING;
                    const double M_MAX_PKG_POWER_SETTING;
            };

            class LeafRole : public Role {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    LeafRole(PlatformIO &platform_io,
                             const PlatformTopo &platform_topo,
                             std::shared_ptr<SampleAggregator> sample_agg,
                             std::vector<std::shared_ptr<PowerBalancer> > power_balancer,
                             double min_power,
                             double max_power,
                             double time_window,
                             bool is_single_node,
                             int num_node);
                    virtual ~LeafRole();
                    bool adjust_platform(const std::vector<double> &in_policy) override;
                    bool sample_platform(std::vector<double> &out_sample) override;
                    void trace_values(std::vector<double> &values) override;
                private:
                    void init_platform_io(void);
                    void are_steps_complete(bool is_complete);
                    bool are_steps_complete(void);
                    PlatformIO &m_platform_io;
                    const PlatformTopo &m_platform_topo;
                    std::shared_ptr<SampleAggregator> m_sample_agg;
                    /// Number of power control domains
                    int m_num_domain;
                    std::vector<int> m_count_pio_idx;
                    std::vector<int> m_time_agg_idx;
                    std::vector<int> m_network_agg_idx;
                    std::vector<int> m_ignore_agg_idx;
                    std::vector<std::shared_ptr<PowerBalancer> > m_power_balancer;
                    const double M_STABILITY_FACTOR;
                    struct m_package_s {
                        int last_epoch_count;
                        double runtime;
                        double actual_limit;
                        double power_slack;
                        double power_headroom;
                        bool is_out_of_bounds;
                        bool is_step_complete;
                        int pio_power_idx;
                    };
                    std::vector<m_package_s> m_package;
                    const double M_MIN_PKG_POWER_SETTING;
                    const double M_MAX_PKG_POWER_SETTING;
                    bool m_is_single_node;
                    bool m_is_first_policy;
            };
    };
}

#endif
