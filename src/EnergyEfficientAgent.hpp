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

#include "geopm.h"
#include "geopm_time.h"

#include "Agent.hpp"

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;
    class EnergyEfficientRegion;
    class FrequencyGovernor;

    class EnergyEfficientAgent : public Agent
    {
        public:
            EnergyEfficientAgent();
            EnergyEfficientAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                 std::shared_ptr<FrequencyGovernor> gov,
                                 std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> > region_map);
            virtual ~EnergyEfficientAgent() = default;
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
            void trace_values(std::vector<double> &values) override;
            void enforce_policy(const std::vector<double> &policy) const override;

            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            bool update_freq_range(const std::vector<double> &in_policy);
            void init_platform_io(void);

            enum m_policy_e {
                M_POLICY_FREQ_MIN,
                M_POLICY_FREQ_MAX,
                M_NUM_POLICY,
            };

            enum m_signal_e {
                M_SIGNAL_REGION_HASH,
                M_SIGNAL_REGION_HINT,
                M_SIGNAL_REGION_RUNTIME,
                M_NUM_SIGNAL,
            };

            class DomainContext {
                public:
                    DomainContext(std::shared_ptr<FrequencyGovernor> gov);
                    virtual ~DomainContext() = default;
                    double target_freq(uint64_t region_hash);
                    void exit(uint64_t region_hash, double perf_metric);
                    bool is_region_boundary(uint64_t region_hash);
                    void last_frequency(uint64_t region_hash, double frequency);
                    struct geopm_region_info_s last_region_info(void);
                    void last_region_info(struct geopm_region_info_s region_info);
                    //@todo move to private when I have a proper story for the usage of this
                    std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> > m_region_map;
                private:
                    std::shared_ptr<FrequencyGovernor> m_freq_governor;
                    struct geopm_region_info_s  m_last_region;
            };

            const int M_PRECISION;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<FrequencyGovernor> m_freq_governor;
            int m_freq_ctl_domain_type;
            int m_num_freq_ctl_domain;
            std::vector<DomainContext> m_domain_ctx;
            geopm_time_s m_last_wait;
            std::vector<std::vector<int> > m_signal_idx;
            int m_level;
            int m_num_children;
            bool m_do_send_policy;
    };
}

#endif
