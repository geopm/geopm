/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef KPROFILEIOGROUP_HPP_INCLUDE
#define KPROFILEIOGROUP_HPP_INCLUDE

#include <set>
#include <string>

#include "geopm_time.h"
#include "IOGroup.hpp"

namespace geopm
{
    class IEpochRuntimeRegulator;
    class IKprofileIOSample;
    class IPlatformTopo;

    /// @brief IOGroup that provides signals from the application.
    class KprofileIOGroup : public IOGroup
    {
        public:
            KprofileIOGroup(std::shared_ptr<IKprofileIOSample> profile_sample,
                            IEpochRuntimeRegulator &epoch_regulator);
            KprofileIOGroup(std::shared_ptr<IKprofileIOSample> profile_sample,
                            IEpochRuntimeRegulator &epoch_regulator,
                            geopm::IPlatformTopo &topo);
            virtual ~KprofileIOGroup();
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int signal_idx) const override;
            void adjust(int control_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) const override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            static std::string plugin_name(void);
        private:
            enum m_signal_type {
                M_SIGNAL_REGION_ID,
                M_SIGNAL_PROGRESS,
                M_SIGNAL_EPOCH_RUNTIME,
                M_SIGNAL_EPOCH_COUNT,
                M_SIGNAL_RUNTIME,
                M_SIGNAL_MAX,
            };
            struct m_signal_config {
                int signal_type;
                int domain_type;
                int domain_idx;
            };

            int check_signal(const std::string &signal_name, int domain_type, int domain_idx) const;

            std::shared_ptr<IKprofileIOSample> m_profile_sample;
            IEpochRuntimeRegulator &m_epoch_regulator;
            std::map<std::string, int> m_signal_idx_map;
            IPlatformTopo &m_platform_topo;
            std::vector<bool> m_do_read;
            bool m_is_batch_read = false;
            std::vector<struct m_signal_config> m_active_signal;
            struct geopm_time_s m_read_time;
            std::vector<uint64_t> m_per_cpu_region_id;
            std::vector<double> m_per_cpu_progress;
            std::vector<double> m_per_cpu_runtime;
            std::vector<double> m_epoch_runtime;
            std::vector<double> m_epoch_count;
            std::map<int, int> m_rid_idx; // map from runtime signal index to the region id signal it uses
    };
}

#endif
