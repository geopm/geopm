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

#ifndef POWERGOVERNORAGENT_HPP_INCLUDE
#define POWERGOVERNORAGENT_HPP_INCLUDE

#include <vector>
#include <functional>

#include "Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
    template <class type>
    class ICircularBuffer;
    class IPowerGovernor;

    class PowerGovernorAgent : public Agent
    {
        public:
            enum m_policy_e {
                M_POLICY_POWER,
                M_NUM_POLICY,
            };
            enum m_plat_signal_e {
                M_PLAT_SIGNAL_PKG_POWER,
                M_PLAT_SIGNAL_DRAM_POWER,
                M_PLAT_NUM_SIGNAL,
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

            PowerGovernorAgent();
            PowerGovernorAgent(IPlatformIO &platform_io, IPlatformTopo &platform_topo, std::unique_ptr<IPowerGovernor> power_gov);
            virtual ~PowerGovernorAgent();
            void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
            std::vector<double> validate_policy(const std::vector<double> &in_policy) const override;
            bool descend(const std::vector<double> &in_policy,
                         std::vector<std::vector<double> >&out_policy) override;
            bool ascend(const std::vector<std::vector<double> > &in_sample,
                        std::vector<double> &out_sample) override;
            bool adjust_platform(const std::vector<double> &in_policy) override;
            bool sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::vector<std::pair<std::string, std::string> > report_header(void) const override;
            std::vector<std::pair<std::string, std::string> > report_node(void) const override;
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
            std::vector<std::string> trace_names(void) const override;
            void trace_values(std::vector<double> &values) override;
            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            void init_platform_io(void);

            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;

            int m_level;
            bool m_is_converged;
            bool m_is_sample_stable;
            double m_min_power_setting;
            double m_max_power_setting;
            double m_tdp_power_setting;
            std::unique_ptr<IPowerGovernor> m_power_gov;
            std::vector<int> m_pio_idx;
            std::vector<std::function<double(const std::vector<double>&)> > m_agg_func;
            int m_num_children;
            double m_last_power_budget;
            std::unique_ptr<ICircularBuffer<double> > m_epoch_power_buf;
            std::vector<double> m_sample;
            int m_ascend_count;
            const int m_ascend_period;
            const int m_min_num_converged;
            int m_num_pkg;
            double m_adjusted_power;
            geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
    };
}

#endif
