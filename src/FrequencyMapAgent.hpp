/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef FREQUENCYMAPAGENT_HPP_INCLUDE
#define FREQUENCYMAPAGENT_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include "geopm_time.h"

#include "Agent.hpp"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    class FrequencyMapAgent : public Agent
    {
        public:
            FrequencyMapAgent();
            FrequencyMapAgent(PlatformIO &plat_io, const PlatformTopo &topo);
            virtual ~FrequencyMapAgent() = default;
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
            void update_policy(const std::vector<double> &policy);
            void init_platform_io(void);
            static bool is_all_nan(const std::vector<double> &vec);

            enum m_policy_e {
                M_POLICY_FREQ_DEFAULT,
                M_POLICY_FREQ_UNCORE,
                M_POLICY_FIRST_HASH,
                M_POLICY_FIRST_FREQUENCY,
                // The remainder of policy values can be additional pairs of
                // (hash, frequency)
                M_NUM_POLICY = 64,
            };

            const int M_PRECISION;
            const double M_WAIT_SEC;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            geopm_time_s m_wait_time;
            std::map<uint64_t, double> m_hash_freq_map;
            std::vector<int> m_hash_signal_idx;
            std::vector<int> m_freq_control_idx;
            int m_uncore_min_ctl_idx;
            int m_uncore_max_ctl_idx;
            std::vector<uint64_t> m_last_hash;
            std::vector<double> m_last_freq;
            double m_last_uncore_freq;
            int m_num_children;
            bool m_is_policy_updated;
            bool m_do_write_batch;
            bool m_is_adjust_initialized;
            bool m_is_real_policy;
            int m_freq_ctl_domain_type;
            int m_num_freq_ctl_domain;
            double m_core_freq_min;
            double m_core_freq_max;
            double m_uncore_init_min;
            double m_uncore_init_max;
            double m_default_freq;
            double m_uncore_freq;
    };
}

#endif
