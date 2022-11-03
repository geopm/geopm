/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPUACTIVITYAGENT_HPP_INCLUDE
#define CPUACTIVITYAGENT_HPP_INCLUDE

#include <vector>

#include "geopm_time.h"
#include "Agent.hpp"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
    class FrequencyGovernor;

    /// @brief Agent
    class CPUActivityAgent : public Agent
    {
        public:
            CPUActivityAgent();
            CPUActivityAgent(PlatformIO &plat_io,
                             const PlatformTopo &topo,
                             std::shared_ptr<FrequencyGovernor> gov);
            virtual ~CPUActivityAgent() = default;
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
            void enforce_policy(const std::vector<double> &policy) const override;
            std::vector<std::function<std::string(double)> > trace_formats(void) const override;

            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            geopm_time_s m_last_wait;
            double M_WAIT_SEC;
            const double M_POLICY_PHI_DEFAULT;
            const int M_NUM_PACKAGE;
            bool m_do_write_batch;
            bool m_do_send_policy;
            std::shared_ptr<FrequencyGovernor> m_freq_governor;
            int m_freq_ctl_domain_type;
            int m_num_freq_ctl_domain;
            double m_core_batch_writes;
            double m_uncore_frequency_requests;
            double m_uncore_frequency_clamped;
            double m_resolved_f_uncore_efficient;
            double m_resolved_f_uncore_max;
            double m_resolved_f_core_efficient;
            double m_resolved_f_core_max;
            double m_freq_uncore_min;
            double m_freq_uncore_max;
            double m_freq_uncore_efficient;
            double m_freq_core_min;
            double m_freq_core_max;
            double m_freq_core_efficient;

            struct signal
            {
                int batch_idx;
                double value;
            };

            struct control
            {
                int batch_idx;
                double last_setting;
            };

            // Policy indices; must match policy_names()
            enum m_policy_e {
                M_POLICY_CPU_PHI,
                M_NUM_POLICY,
            };

            // Sample indices; must match sample_names()
            enum m_sample_e {
                M_NUM_SAMPLE
            };

            std::map<std::string, double> m_policy_available;
            // Maps uncore frequency -> maximum memory bandwidth achieved by
            // that frequency (determined by system characterization)
            std::map<double, double> m_qm_max_rate;

            std::vector<signal> m_core_scal;
            std::vector<control> m_core_freq_control;

            std::vector<signal> m_qm_rate;
            std::vector<signal> m_uncore_freq_status;
            std::vector<control> m_uncore_freq_min_control;
            std::vector<control> m_uncore_freq_max_control;

            void init_platform_io(void);
            void init_constconfig_io(void);
    };
}
#endif
