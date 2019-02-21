/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
    class EnergyEfficientRegion;

    class EnergyEfficientAgent : public Agent
    {
        public:
            EnergyEfficientAgent();
            EnergyEfficientAgent(IPlatformIO &plat_io, IPlatformTopo &topo);
            virtual ~EnergyEfficientAgent() = default;
            void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
            void validate_policy(std::vector<double> &policy) const override;
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
            friend class IMode;
            class IMode {
                public:
                    IMode() = default;
                    virtual ~IMode() = default;
                    virtual void init_platform_io(std::vector<std::string> &signal_names) = 0;
                    virtual void post_policy_change(EnergyEfficientAgent &ctx) = 0;
                    virtual double select_frequency(EnergyEfficientAgent &ctx) = 0;
                    virtual void post_sample_platform(EnergyEfficientAgent &ctx, uint64_t current_region_hash, uint64_t current_region_hint) = 0;
                    virtual std::vector<std::pair<std::string, std::string> > report_node(void) const = 0;
                    virtual std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const = 0;
                    virtual std::vector<std::string> trace_names(void) const = 0;
                    virtual void trace_values(EnergyEfficientAgent &ctx, std::vector<double> &values) = 0;
            };
            class OnlineMode : public IMode {
                public:
                    OnlineMode() = default;
                    virtual ~OnlineMode() = default;
                    void init_platform_io(std::vector<std::string> &signal_names) override;
                    void post_policy_change(EnergyEfficientAgent &ctx) override;
                    double select_frequency(EnergyEfficientAgent &ctx) override;
                    void post_sample_platform(EnergyEfficientAgent &ctx, uint64_t current_region_hash, uint64_t current_region_hint) override;
                    std::vector<std::pair<std::string, std::string> > report_node(void) const override;
                    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
                    std::vector<std::string> trace_names(void) const override;
                    void trace_values(EnergyEfficientAgent &ctx, std::vector<double> &values) override;
                private:
                    std::map<uint64_t, double> m_adapt_freq_map;
                    std::map<uint64_t, std::unique_ptr<EnergyEfficientRegion> > m_region_map;
            };
            class OfflineMode : public IMode {
                public:
                    OfflineMode();
                    virtual ~OfflineMode() = default;
                    void init_platform_io(std::vector<std::string> &signal_names) override;
                    void post_policy_change(EnergyEfficientAgent &ctx) override;
                    double select_frequency(EnergyEfficientAgent &ctx) override;
                    void post_sample_platform(EnergyEfficientAgent &ctx, uint64_t current_region_hash, uint64_t current_region_hint) override;
                    std::vector<std::pair<std::string, std::string> > report_node(void) const override;
                    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
                    std::vector<std::string> trace_names(void) const override;
                    void trace_values(EnergyEfficientAgent &ctx, std::vector<double> &values) override;
                private:
                    void parse_env_map(void);
                    std::map<uint64_t, double> m_hash_freq_map;
            };
            bool update_policy(const std::vector<double> &policy);
            double get_limit(const std::string &sig_name) const;
            void init_platform_io(void);

            enum m_policy_e {
                M_POLICY_FREQ_MIN,
                M_POLICY_FREQ_MAX,
                M_NUM_POLICY,
            };

            enum m_sample_e {
                M_SAMPLE_ENERGY_PACKAGE,
                M_SAMPLE_FREQUENCY,
                M_NUM_SAMPLE,
            };

            enum m_signal_e {
                M_SIGNAL_ENERGY_PACKAGE,
                M_SIGNAL_FREQUENCY,
                M_SIGNAL_REGION_HASH,
                M_SIGNAL_REGION_HINT,
                M_SIGNAL_RUNTIME,
                M_NUM_SIGNAL,
            };

            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            double m_freq_min;
            double m_freq_max;
            const double M_FREQ_STEP;
            const size_t M_SEND_PERIOD;
            std::vector<int> m_control_idx;
            double m_last_freq;
            std::pair<uint64_t, uint64_t> m_last_region;
            std::unique_ptr<IMode> m_mode;
            geopm_time_s m_last_wait;
            std::vector<int> m_sample_idx;
            std::vector<int> m_signal_idx;
            std::vector<std::function<double(const std::vector<double>&)> > m_agg_func;
            int m_level;
            int m_num_children;
            size_t m_num_ascend;
    };
}

#endif
