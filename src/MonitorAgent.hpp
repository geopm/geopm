/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MONITORAGENT_HPP_INCLUDE
#define MONITORAGENT_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include "Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    /// @brief Agent used to do sampling only; no policy will be enforced.
    class MonitorAgent : public Agent
    {
        public:
            MonitorAgent();
            MonitorAgent(PlatformIO &plat_io, const PlatformTopo &topo);
            virtual ~MonitorAgent() = default;
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

            /// @return "energy_efficient"
            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            /// @return a list of policy names
            static std::vector<std::string> policy_names(void);
            /// @return a list of sample names
            static std::vector<std::string> sample_names(void);
        private:
            struct geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
    };
}

#endif
