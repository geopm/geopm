/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GPUACTIVITYAGENT_HPP_INCLUDE
#define GPUACTIVITYAGENT_HPP_INCLUDE

#include <functional>
#include <vector>

#include "Agent.hpp"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
    class Waiter;

    /// @brief Agent
    class GPUActivityAgent : public Agent
    {
        public:
            GPUActivityAgent();
            GPUActivityAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                             std::shared_ptr<Waiter> waiter);
            virtual ~GPUActivityAgent() = default;
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
            static constexpr double M_WAIT_SEC = 0.020; // 20ms wait default
            const double M_POLICY_PHI_DEFAULT;
            const int M_NUM_GPU;
            const int M_NUM_GPU_CHIP;
            const int M_NUM_CHIP_PER_GPU;
            bool m_do_write_batch;
            bool m_do_send_policy;

            int m_agent_domain_count;
            int m_agent_domain;

            struct m_signal
            {
                int batch_idx;
                double value;
            };

            struct m_control
            {
                int batch_idx;
                double last_setting;
            };

            // Policy indices; must match policy_names()
            enum m_policy_e {
                M_POLICY_GPU_PHI,
                M_NUM_POLICY
            };

            // Sample indices; must match sample_names()
            enum m_sample_e {
                M_NUM_SAMPLE
            };

            std::map<std::string, double> m_policy_available;

            double m_gpu_frequency_requests;
            double m_gpu_frequency_clipped;
            double m_freq_gpu_min;
            double m_freq_gpu_max;
            double m_freq_gpu_efficient;
            double m_resolved_f_gpu_max;
            double m_resolved_f_gpu_efficient;
            double m_f_range;

            std::vector<m_signal> m_gpu_core_activity;
            std::vector<m_signal> m_gpu_utilization;
            std::vector<m_signal> m_gpu_energy;
            m_signal m_time;

            std::vector<m_control> m_gpu_freq_min_control;
            std::vector<m_control> m_gpu_freq_max_control;
            std::shared_ptr<Waiter> m_waiter;

            void init_platform_io(void);
    };
}
#endif
