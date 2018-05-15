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

#ifndef ENERGYEFFICIENTAGENT_HPP_INCLUDE
#define ENERGYEFFICIENTAGENT_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include "geopm_time.h"

#include "Agent.hpp"
#include "EnergyEfficientRegion.hpp"

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;

    class EnergyEfficientAgent : public Agent
    {
        public:
            EnergyEfficientAgent();
            EnergyEfficientAgent(IPlatformIO &plat_io, IPlatformTopo &topo);
            virtual ~EnergyEfficientAgent() = default;
            void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
            bool descend(const std::vector<double> &in_policy,
                         std::vector<std::vector<double> >&out_policy) override;
            bool ascend(const std::vector<std::vector<double> > &in_sample,
                        std::vector<double> &out_sample) override;
            bool adjust_platform(const std::vector<double> &in_policy) override;
            bool sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::vector<std::pair<std::string, std::string> > report_header(void) override;
            std::vector<std::pair<std::string, std::string> > report_node(void) override;
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) override;
            std::vector<std::string> trace_names(void) const;
            void trace_values(std::vector<double> &values);

            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            double cpu_freq_min(void) const;
            double cpu_freq_max(void) const;
            double get_limit(const std::string &sig_name) const;
            void init_platform_io(void);
            void parse_env_map(void);

            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            const double M_FREQ_MIN;
            const double M_FREQ_MAX;
            const double M_FREQ_STEP;
            const size_t M_SEND_PERIOD;
            std::vector<int> m_control_idx;
            double m_last_freq;
            double m_curr_adapt_freq;
            std::map<uint64_t, double> m_rid_freq_map;
            // for online adaptive mode
            bool m_is_adaptive = false;
            std::map<uint64_t, std::unique_ptr<EnergyEfficientRegion> > m_region_map;
            geopm_time_s m_last_wait;
            std::vector<int> m_sample_idx;
            std::vector<std::function<double(const std::vector<double>&)> > m_agg_func;
            size_t m_num_sample;
            int m_level;
            uint64_t m_last_region_id = 0;
            size_t m_num_ascend = 0;
            int m_region_id_idx;
            int m_runtime_idx;
            int m_pkg_energy_idx;
            int m_dram_energy_idx;
    };
}

#endif
