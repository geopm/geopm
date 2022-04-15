/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FREQUENCYMAPAGENT_HPP_INCLUDE
#define FREQUENCYMAPAGENT_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <set>

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
            FrequencyMapAgent(const std::map<uint64_t, double>& hash_freq_map,
                              const std::set<uint64_t>& default_freq_hash,
                              PlatformIO &plat_io, const PlatformTopo &topo);
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
            std::set<uint64_t> m_default_freq_hash;
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
