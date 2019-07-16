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
#ifndef CNLIOGROUP_HPP_INCLUDE
#define CNLIOGROUP_HPP_INCLUDE

#include <functional>
#include <map>

#include "IOGroup.hpp"
#include "geopm_time.h"

namespace geopm
{
    /// @brief IOGroup that wraps interfaces to Compute Node Linux.
    class CNLIOGroup : public IOGroup
    {
        public:
            CNLIOGroup();
            CNLIOGroup(const std::string &pm_counters_path);
            virtual ~CNLIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type,
                            int domain_idx) override;
            int push_control(const std::string &control_name, int domain_type,
                             int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type,
                               int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)>
                agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)>
                format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            enum signal_type_e {
                SIGNAL_TYPE_POWER_BOARD,
                SIGNAL_TYPE_ENERGY_BOARD,
                SIGNAL_TYPE_POWER_MEMORY,
                SIGNAL_TYPE_ENERGY_MEMORY,
                SIGNAL_TYPE_POWER_CPU,
                SIGNAL_TYPE_ENERGY_CPU,
                SIGNAL_TYPE_SAMPLE_RATE,
                SIGNAL_TYPE_ELAPSED_TIME,
                NUM_SIGNAL_TYPE
            };
            struct signal_s {
                const std::string m_description;
                const std::function<double(const std::vector<double> &)> m_agg_function;
                const std::function<std::string(double)> m_format_function;
                const std::function<double()> m_read_function;
                bool m_do_read;
                double m_value;
            };

            double read_time(const std::string &freshness_path) const;

            geopm_time_s m_time_zero;
            double m_initial_freshness;
            double m_sample_rate;
            std::map<std::string, signal_type_e> m_signal_offsets;
            std::vector<signal_s> m_signals;
    };
}

#endif
