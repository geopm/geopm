/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EXAMPLEAGENT_HPP_INCLUDE
#define EXAMPLEAGENT_HPP_INCLUDE

#include <vector>

#include "geopm/Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
    class Waiter;
}

/// @brief Agent
class ExampleAgent : public geopm::Agent
{
    public:
        ExampleAgent();
        virtual ~ExampleAgent() = default;
        void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
        void validate_policy(std::vector<double> &in_policy) const override;
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
        void trace_values(std::vector<double> &values) override;
        std::vector<std::function<std::string(double)> > trace_formats(void) const override;
        void enforce_policy(const std::vector<double> &policy) const override;

        static std::string plugin_name(void);
        static std::unique_ptr<geopm::Agent> make_plugin(void);
        static std::vector<std::string> policy_names(void);
        static std::vector<std::string> sample_names(void);
    private:
        void load_trace_columns(void);

        // Policy indices; must match policy_names()
        enum m_policy_e {
            M_POLICY_LOW_THRESH,
            M_POLICY_HIGH_THRESH,
            M_NUM_POLICY
        };
        // Sample indices; must match sample_names()
        enum m_sample_e {
            M_SAMPLE_USER_PCT,
            M_SAMPLE_SYSTEM_PCT,
            M_SAMPLE_IDLE_PCT,
            M_NUM_SAMPLE
        };
        // Signals read in sample_platform()
        enum m_plat_signal_e {
            M_PLAT_SIGNAL_USER,
            M_PLAT_SIGNAL_SYSTEM,
            M_PLAT_SIGNAL_IDLE,
            M_PLAT_SIGNAL_NICE,
            M_NUM_PLAT_SIGNAL
        };
        // Controls written in adjust_platform()
        enum m_plat_control_e {
            M_PLAT_CONTROL,
            M_NUM_PLAT_CONTROL
        };
        // Values for trace
        enum m_trace_value_e {
            M_TRACE_VAL_USER_PCT,
            M_TRACE_VAL_SYSTEM_PCT,
            M_TRACE_VAL_IDLE_PCT,
            M_TRACE_VAL_SIGNAL_USER,
            M_TRACE_VAL_SIGNAL_SYSTEM,
            M_TRACE_VAL_SIGNAL_IDLE,
            M_TRACE_VAL_SIGNAL_NICE,
            M_NUM_TRACE_VAL
        };

        geopm::PlatformIO &m_platform_io;
        const geopm::PlatformTopo &m_platform_topo;

        std::vector<int> m_signal_idx;
        std::vector<int> m_control_idx;
        std::vector<double> m_last_sample;
        std::vector<double> m_last_signal;

        const double M_WAIT_SEC;
        std::unique_ptr<geopm::Waiter> m_waiter;

        double m_min_idle;
        double m_max_idle;

        bool m_is_control_active;
};

#endif
