/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FIXEDFREQUENCYAGENT_HPP_INCLUDE
#define FIXEDFREQUENCYAGENT_HPP_INCLUDE

#include <vector>

#include "Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;

    /// @brief Agent
    class FixedFrequencyAgent : public Agent
    {
        public:
            FixedFrequencyAgent();
            FixedFrequencyAgent(PlatformIO &plat_io, const PlatformTopo &topo);
            virtual ~FixedFrequencyAgent() = default;
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
            // Policy indices; must match policy_names()
            enum m_policy_e {
                M_POLICY_GPU_FREQUENCY,
                M_POLICY_CPU_FREQUENCY,
                M_POLICY_UNCORE_MIN_FREQUENCY,
                M_POLICY_UNCORE_MAX_FREQUENCY,
                M_POLICY_SAMPLE_PERIOD,
                M_NUM_POLICY
            };
            // Sample indices; must match sample_names()
            enum m_sample_e {
                M_NUM_SAMPLE
            };

            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;

            geopm_time_s m_last_wait;
            double M_WAIT_SEC;
            bool m_is_adjust_initialized;
    };
}

#endif
