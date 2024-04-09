/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MONITORAGENT_HPP_INCLUDE
#define MONITORAGENT_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include "geopm/Agent.hpp"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;
    class Waiter;

    /// @brief Agent used to do sampling only; no policy will be enforced.
    class MonitorAgent : public Agent
    {
        public:
            MonitorAgent();
            MonitorAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                         std::shared_ptr<Waiter> waiter);
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

            /// @return "monitor"
            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            /// @return a list of policy names
            static std::vector<std::string> policy_names(void);
            /// @return a list of sample names
            static std::vector<std::string> sample_names(void);
            static constexpr double M_WAIT_SEC = 0.2; // 200 msec
        private:
            std::shared_ptr<Waiter> m_waiter;

    };
}

#endif
