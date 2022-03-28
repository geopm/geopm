/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef GPUACTIVITYAGENT_HPP_INCLUDE
#define GPUACTIVITYAGENT_HPP_INCLUDE

#include <functional>
#include <vector>

#include "geopm_time.h"
#include "Agent.hpp"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;

    /// @brief Agent
    class GPUActivityAgent : public Agent
    {
        public:
            GPUActivityAgent();
            GPUActivityAgent(PlatformIO &plat_io, const PlatformTopo &topo);
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
            const double M_GPU_ACTIVITY_CUTOFF;
            const int M_NUM_GPU;
            bool m_do_write_batch;

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
                M_POLICY_GPU_FREQ_MAX,
                M_POLICY_GPU_FREQ_EFFICIENT,
                M_POLICY_GPU_PHI,
                M_POLICY_SAMPLE_PERIOD,
                M_NUM_POLICY
            };

            // Sample indices; must match sample_names()
            enum m_sample_e {
                M_NUM_SAMPLE
            };

            std::map<std::string, double> m_policy_available;

            double m_gpu_frequency_requests;
            double m_gpu_frequency_clipped;
            double m_f_max;
            double m_f_efficient;
            double m_f_range;
            std::vector<double> m_gpu_active_region_start;
            std::vector<double> m_gpu_active_region_stop;
            std::vector<double> m_gpu_active_energy_start;
            std::vector<double> m_gpu_active_energy_stop;

            std::vector<signal> m_gpu_freq_status;
            std::vector<signal> m_gpu_compute_activity;
            std::vector<signal> m_gpu_utilization;
            std::vector<signal> m_gpu_energy;
            signal m_time;

            std::vector<control> m_gpu_freq_control;

            void init_platform_io(void);
    };
}
#endif
