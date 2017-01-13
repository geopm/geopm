/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef TELEMETRYCONFIG_HPP_INCLUDE
#define TELEMETRYCONFIG_HPP_INCLUDE

#include <vector>
#include <map>
#include <string>

namespace geopm {

    enum aggregation_op_type_e {
        AGGREGATION_OP_SUM,
        AGGREGATION_OP_AVG,
        AGGREGATION_OP_MIN,
        AGGREGATION_OP_MAX,
    };

    class TelemetryConfig {
        public:
            TelemetryConfig(const std::vector<int> &fan_out);
            TelemetryConfig(const TelemetryConfig &other);
            virtual ~TelemetryConfig();
            void set_provided(int signal_domain, const std::vector<std::string> &provided);
            void get_provided(int signal_domain, std::vector<std::string> &provided) const;
            bool is_provided(int signal_domain, const std::string &signal) const;
            void set_required(int signal_domain, const std::vector<std::string> &required);
            void set_required(int signal_domain, const std::string &required);
            void get_required(int signal_domain, std::vector<std::string> &required) const;
            void get_required(std::map<int, std::vector<std::string> > &required) const;
            bool is_required(int signal_domain, const std::string &signal) const;
            void set_domain_cpu_map(int domain, const std::vector<std::vector<int> > &domain_map);
            void get_domain_cpu_map(int domain, std::vector<std::vector<int> > &domain_map) const;
            void num_signal_per_domain(std::vector<int> &num_signal) const;
            int num_signal_per_domain(int domain) const;
            int num_required_signal(void) const;
            void set_bounds(int signal_domain, double lower, double upper);
            void get_bounds(int level, int ctl_domain, double &lower, double &upper) const;
            void supported_domain(const std::vector<int> domain);
            bool is_supported_domain(int domain) const;
            void set_aggregate(const std::vector<std::pair<std::string, std::pair<int, int> > > &agg);
            void set_aggregate(std::string signal, int spacial_op_type, int temporal_op_type);
            void get_aggregate(std::vector<std::pair<std::string, std::pair<int, int> > > &agg) const;
            int num_aggregated_signal(void) const;
        private:
            int num_children(int level);
            std::map<int, std::vector<std::string> > m_provided_signal;
            std::map<int, std::vector<std::string> > m_required_signal;
            std::vector<std::pair<std::string, std::pair<int, int> > > m_aggregate_signal;
            std::map<int, std::pair<double, double> > m_control_bound;
            std::map<int, std::vector<std::vector<int> > > m_domain_map;
            std::vector<int> m_supported_domain;
            std::vector<int> m_fan_out;
    };
}
#endif
