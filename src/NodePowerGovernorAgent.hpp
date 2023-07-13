/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NODEPOWERGOVERNORAGENT_HPP_INCLUDE
#define NODEPOWERGOVERNORAGENT_HPP_INCLUDE

#include <vector>
#include <functional>

#include "Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;
    template <class type>
    class CircularBuffer;
    class PowerGovernor;

    class NodePowerGovernorAgent : public Agent
    {
        public:
            enum m_policy_e {
                M_POLICY_POWER,
                M_NUM_POLICY,
            };
            enum m_plat_signal_e {
                M_PLAT_SIGNAL_NODE_POWER,
                M_PLAT_NUM_SIGNAL,
            };
            enum m_plat_control_e {
                M_PLAT_CONTROL_NODE_POWER,
                M_PLAT_NUM_CONTROL,
            };
            enum m_trace_sample_e {
                M_TRACE_SAMPLE_PWR_BUDGET,
                M_TRACE_NUM_SAMPLE,
            };
            enum m_sample_e { // Tree samples
                M_SAMPLE_POWER,
                M_SAMPLE_IS_CONVERGED,
                M_SAMPLE_POWER_ENFORCED,
                M_NUM_SAMPLE,
            };

            NodePowerGovernorAgent();
            NodePowerGovernorAgent(PlatformIO &platform_io);
            virtual ~NodePowerGovernorAgent();
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
        private:
            void init_platform_io(void);
            PlatformIO &m_platform_io;
            int m_level;
            bool m_is_sample_stable;
            bool m_do_send_sample;
            double M_MIN_POWER_SETTING;
            double M_MAX_POWER_SETTING;
            double M_POWER_TIME_WINDOW;
            std::unique_ptr<PowerGovernor> m_power_gov;
            std::vector<int> m_pio_idx;
            std::vector<int> m_pio_ctl_idx;
            std::vector<std::function<double(const std::vector<double>&)> > m_agg_func;
            int m_num_children;
            bool m_do_write_batch;
            double m_last_power_budget;
            bool m_power_budget_changed;
            std::unique_ptr<CircularBuffer<double> > m_epoch_power_buf;
            std::vector<double> m_sample;
            int m_ascend_count;
            const int m_ascend_period;
            const int m_min_num_converged;
            double m_adjusted_power;
            geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
    };
}

#endif
