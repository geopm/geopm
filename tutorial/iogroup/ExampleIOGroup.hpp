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

#ifndef EXAMPLEIOGROUP_HPP_INCLUDE
#define EXAMPLEIOGROUP_HPP_INCLUDE

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "geopm/PluginFactory.hpp"
#include "geopm/IOGroup.hpp"

namespace geopm
{
    class IPlatformTopo;
}

/// @brief IOGroup that provides a signals for user and idle CPU time, and
///        a control for writing to standard output.
class ExampleIOGroup : public geopm::IOGroup
{
    public:
        ExampleIOGroup();
        virtual ~ExampleIOGroup() = default;
        std::set<std::string> signal_names(void) const override;
        std::set<std::string> control_names(void) const override;
        bool is_valid_signal(const std::string &signal_name) const override;
        bool is_valid_control(const std::string &control_name) const override;
        int signal_domain_type(const std::string &signal_name) const override;
        int control_domain_type(const std::string &control_name) const override;
        int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
        int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
        void read_batch(void) override;
        void write_batch(void) override;
        double sample(int batch_idx) override;
        void adjust(int batch_idx, double setting) override;
        double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
        void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
        void save_control(void) override;
        void restore_control(void) override;
        std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
        std::string signal_description(const std::string &signal_name) const;
        std::string control_description(const std::string &control_name) const;
        static std::string plugin_name(void);
        static std::unique_ptr<geopm::IOGroup> make_plugin(void);
    private:
        std::vector<std::string> parse_proc_stat(void);
        enum m_signal_type_e {
            M_SIGNAL_USER_TIME,
            M_SIGNAL_NICE_TIME,
            M_SIGNAL_SYSTEM_TIME,
            M_SIGNAL_IDLE_TIME,
            M_NUM_SIGNAL
        };
        enum m_control_type_e {
            M_CONTROL_STDOUT,
            M_CONTROL_STDERR,
            M_NUM_CONTROL
        };
        geopm::IPlatformTopo &m_platform_topo;
        /// Whether any signal has been pushed
        bool m_do_batch_read;
        /// Whether read_batch() has been called at least once
        bool m_is_batch_read;
        std::map<std::string, int> m_signal_idx_map;
        std::map<std::string, int> m_control_idx_map;
        std::vector<bool> m_do_read;
        std::vector<bool> m_do_write;
        std::vector<std::string> m_signal_value;
        std::vector<std::string> m_control_value;
};

#endif
