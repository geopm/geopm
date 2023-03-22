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

    /// @brief Agent
    class FFNetAgent : public geopm::Agent
    {
        public:
            enum m_policy_e {
                /// @brief Phi represents the user's desire to trade off
                ///        performance for energy efficiency. A value of
                ///        0 indicates an extreme preference for
                ///        performance and a value of 10 indicates an
                ///        extreme preference for energy efficiency.
                M_POLICY_PHI,
                M_NUM_POLICY,
            };

            FFNetAgent();
            FFNetAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo);
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
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
            std::vector<std::string> trace_names(void) const override;
            std::vector<std::function<std::string(double)> > trace_formats(void) const override;
            void trace_values(std::vector<double> &values) override;
            void enforce_policy(const std::vector<double> &policy) const override;

            static std::string plugin_name(void);
            static std::unique_ptr<geopm::Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            geopm::PlatformIO &m_platform_io;
            const geopm::PlatformTopo &m_platform_topo;
            geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
            bool m_do_write_batch;

            std::map<std::string, double> m_policy_available;

            int m_phi_idx;
            int m_sample;
            bool m_has_gpus;
            std::map<int, std::unique_ptr<DomainNetMap> > m_net_map;
            std::map<int, std::unique_ptr<RegionHintRecommender> > m_freq_recommender;

            // m_freq_control[domain type][domain index] = platform io control index
            std::map<int, std::vector<int> > m_freq_control;
            std::vector<int> m_domain_types;
    };
}
#endif  /* FFNETAGENT_HPP_INCLUDE */
