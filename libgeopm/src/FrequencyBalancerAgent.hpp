/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FREQUENCYBALANCERAGENT_HPP_INCLUDE
#define FREQUENCYBALANCERAGENT_HPP_INCLUDE

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "geopm/Agent.hpp"
#include "geopm/CircularBuffer.hpp"
#include "geopm_time.h"

#include "FrequencyLimitDetector.hpp"
#include "FrequencyTimeBalancer.hpp"

#include <unordered_map>

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;
    class PowerGovernor;
    class FrequencyGovernor;
    class SSTClosGovernor;
    class Waiter;

    class FrequencyBalancerAgent : public Agent
    {
        public:
            FrequencyBalancerAgent();
            FrequencyBalancerAgent(PlatformIO &plat_io,
                                   const PlatformTopo &topo,
                                   std::shared_ptr<Waiter> waiter,
                                   std::shared_ptr<PowerGovernor> power_gov,
                                   std::shared_ptr<FrequencyGovernor> frequency_gov,
                                   std::shared_ptr<SSTClosGovernor> sst_gov,
                                   std::vector<std::shared_ptr<FrequencyTimeBalancer> > package_balancers,
                                   std::shared_ptr<FrequencyLimitDetector> frequency_limit_detector);
            virtual ~FrequencyBalancerAgent() = default;
            void init(int level, const std::vector<int> &fan_in,
                      bool is_level_root) override;
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
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >
                report_region(void) const override;
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

            // Initialize policy-dependent members of this agent
            void initialize_policies(const std::vector<double> &in_policy);

            enum m_policy_e {
                M_POLICY_POWER_PACKAGE_LIMIT_TOTAL,
                M_POLICY_USE_FREQUENCY_LIMITS,
                M_POLICY_USE_SST_TF,
                M_NUM_POLICY,
            };

            static constexpr double M_WAIT_SEC = 0.005;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<Waiter> m_waiter;
            geopm_time_s m_update_time;
            int m_epoch_signal_idx;
            std::vector<int> m_acnt_signal_idx;
            std::vector<int> m_mcnt_signal_idx;
            std::vector<int> m_hash_signal_idx;
            std::vector<int> m_hint_signal_idx;
            std::vector<int> m_time_hint_network_idx;
            std::vector<double> m_last_ctl_frequency;
            std::vector<double> m_last_ctl_clos;
            std::vector<double> m_last_epoch_acnt;
            std::vector<double> m_last_epoch_mcnt;
            std::vector<double> m_last_sample_acnt;
            std::vector<double> m_last_sample_mcnt;
            std::vector<double> m_last_hash;
            std::vector<std::vector<double> > m_last_epoch_frequency;
            std::vector<double> m_current_epoch_max_frequency;
            std::vector<double> m_last_epoch_max_frequency;
            std::vector<std::vector<double> > m_last_epoch_network_time;
            std::vector<std::vector<double> > m_last_epoch_non_network_time_diff;
            std::unordered_map<double, double> m_region_max_observed_frequency;
            double m_last_epoch_time;
            double m_last_epoch_count;
            int m_num_children;
            bool m_is_policy_updated;
            bool m_do_write_batch;
            bool m_is_adjust_initialized;
            bool m_is_real_policy;
            int m_package_count;
            int m_core_count;
            std::vector<std::vector<size_t> > m_package_core_indices;
            double m_policy_power_package_limit_total;
            bool m_policy_use_frequency_limits;
            bool m_use_sst_tf;
            double m_min_power_setting;
            double m_max_power_setting;
            double m_tdp_power_setting;
            double m_frequency_min;
            double m_frequency_sticker;
            double m_frequency_max;
            double m_frequency_step;
            std::shared_ptr<PowerGovernor> m_power_gov;
            std::shared_ptr<FrequencyGovernor> m_freq_governor;
            std::shared_ptr<SSTClosGovernor> m_sst_clos_governor;
            int m_frequency_ctl_domain_type;
            int m_frequency_control_domain_count;
            std::vector<long long> m_network_hint_sample_length;
            std::vector<long long> m_non_network_hint_sample_length;
            std::vector<size_t> m_last_hp_count;
            bool m_handle_new_epoch;
            int m_epoch_wait_count;
            /* One balancer per package */
            std::vector<std::shared_ptr<FrequencyTimeBalancer> > m_package_balancers;
            std::shared_ptr<FrequencyLimitDetector> m_frequency_limit_detector;
    };
}

#endif
