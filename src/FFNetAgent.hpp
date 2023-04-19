/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FFNETAGENT_HPP_INCLUDE
#define FFNETAGENT_HPP_INCLUDE

#include <vector>

#include "Agent.hpp"
#include "geopm_time.h"
#include "DomainNetMap.hpp"
#include "RegionHintRecommender.hpp"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;

    /// @brief Feed Forward Net Agent
    class FFNetAgent : public Agent
    {
        public:
            enum m_policy_e {
                /// @brief Perf-energy-bias represents the user's desire 
                ///        to trade off performance for energy efficiency. 
                ///        A value of 0 indicates an extreme preference for
                ///        performance and a value of 1 indicates an
                ///        extreme preference for energy efficiency.
                M_POLICY_PERF_ENERGY_BIAS,
                M_NUM_POLICY,
            };

            FFNetAgent();
            FFNetAgent(
                    PlatformIO &plat_io,
                    const PlatformTopo &topo,
                    const std::map<std::pair<geopm_domain_e, int>, std::shared_ptr<DomainNetMap> >
                        &net_map,
                    const std::map<geopm_domain_e, std::shared_ptr<RegionHintRecommender> >
                        &freq_recommender
            );
            virtual ~FFNetAgent() = default;
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
            struct domain_key_s {
                geopm_domain_e type;
                int index;
                bool operator<(const domain_key_s &other) const {
                    return type < other.type || (type == other.type && index < other.index);
                }
            };
            struct control_s {
                int max_idx;
                int min_idx;
                double last_value;
            };

            static bool is_all_nan(const std::vector<double> &vec);
            void init_domain_indices(const PlatformTopo &topo);

            PlatformIO &m_platform_io;
            geopm_time_s m_last_wait;
            static constexpr double M_WAIT_SEC = 0.020;
            bool m_do_write_batch;

            std::map<std::string, double> m_policy_available;

            double m_perf_energy_bias;
            int m_sample;
            std::map<domain_key_s, std::shared_ptr<DomainNetMap> > m_net_map;
            std::map<geopm_domain_e, std::shared_ptr<RegionHintRecommender> > m_freq_recommender;

            std::map<domain_key_s, control_s> m_freq_control;
            std::vector<geopm_domain_e> m_domain_types;
            std::vector<domain_key_s> m_domains;

            static const std::map<geopm_domain_e, const char *> M_NNET_ENVNAME;
            static const std::map<geopm_domain_e, const char *> M_FREQMAP_ENVNAME;
            static const std::map<geopm_domain_e, std::string> M_MAX_FREQ_SIGNAL_NAME;
            static const std::map<geopm_domain_e, std::string> M_MIN_FREQ_SIGNAL_NAME;
            static const std::map<geopm_domain_e, std::string> M_MAX_FREQ_CONTROL_NAME;
            static const std::map<geopm_domain_e, std::string> M_MIN_FREQ_CONTROL_NAME;
            static const std::map<geopm_domain_e, std::string> M_TRACE_SUFFIX;
    };
}
#endif  /* FFNETAGENT_HPP_INCLUDE */
