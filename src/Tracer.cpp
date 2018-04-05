/*
 * Copyright (c) 2016, Intel Corporation
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

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "Tracer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "geopm_env.h"
#include "config.h"

using geopm::IPlatformTopo;

namespace geopm
{
    Tracer::Tracer(std::string header)
        : m_header(header)
        , m_is_trace_enabled(false)
        , m_do_header(true)
        , m_buffer_limit(134217728) // 128 MiB
        , m_time_zero({{0, 0}})
        , m_policy({0, 0, 0, 0.0})
        , m_platform_io(platform_io())
    {
        geopm_time(&m_time_zero);
        if (geopm_env_do_trace()) {
            char hostname[NAME_MAX];
            int err = gethostname(hostname, NAME_MAX);
            if (err) {
                throw Exception("Tracer::Tracer() gethostname() failed", err, __FILE__, __LINE__);
            }
            m_hostname = hostname;
            std::ostringstream output_path;
            output_path << geopm_env_trace() << "-" << m_hostname;
            m_stream.open(output_path.str());
            if (!m_stream.good()) {
                std::cerr << "Warning: unable to open trace file '" << output_path.str()
                          << "': " << strerror(errno) << std::endl;
            }
            m_buffer << std::setprecision(16);
            m_is_trace_enabled = true;
        }
    }

    std::string Tracer::hostname(void) {
        char hostname[NAME_MAX];
        int err = gethostname(hostname, NAME_MAX);
        if (err) {
            throw Exception("Tracer::hostname() gethostname() failed", err, __FILE__, __LINE__);
        }
        return hostname;
    }

    Tracer::Tracer()
        : Tracer(geopm_env_trace(), hostname(), geopm_env_do_trace(), platform_io())
    {

    }

    Tracer::Tracer(const std::string &file_path,
                   const std::string &hostname,
                   bool do_trace,
                   IPlatformIO &platform_io)
        : m_file_path(file_path)
        , m_hostname(hostname)
        , m_is_trace_enabled(do_trace)
        , m_do_header(true)
        , m_buffer_limit(134217728) // 128 MiB
        , m_time_zero({{0, 0}})
        , m_policy({0, 0, 0, 0.0})
        , m_platform_io(platform_io)
    {
        if (m_is_trace_enabled) {
            std::ostringstream output_path;
            output_path << m_file_path << "-" << m_hostname;
            m_stream.open(output_path.str());
            if (!m_stream.good()) {
                std::cerr << "Warning: unable to open trace file '" << output_path.str()
                          << "': " << strerror(errno) << std::endl;
            }
            m_buffer << std::setprecision(16);

            m_header = "HEADER";
            m_buffer << m_header;
            m_buffer << "# \"node_name\" : \"" << m_hostname << "\"" << "\n";
        }
    }

    Tracer::~Tracer()
    {
        if (m_stream.good() && m_is_trace_enabled) {
            m_stream << m_buffer.str();
            m_stream.close();
        }
    }

    void Tracer::update(const std::vector <struct geopm_telemetry_message_s> &telemetry)
    {
        if (m_is_trace_enabled && telemetry.size()) {
            if (m_do_header) {
                // Write the GlobalPolicy information first
                m_buffer << m_header;
                m_buffer << "# \"node_name\" : \"" << m_hostname << "\"" << "\n";
                m_buffer << "region_id | seconds | ";
                for (size_t i = 0; i < telemetry.size(); ++i) {
                    m_buffer << "pkg_energy-" << i << " | "
                             << "dram_energy-" << i << " | "
                             << "frequency-" << i << " | "
                             << "inst_retired-" << i << " | "
                             << "clk_unhalted_core-" << i << " | "
                             << "clk_unhalted_ref-" << i << " | "
                             << "read_bandwidth-" << i << " | "
                             << "progress-" << i << " | "
                             << "runtime-" << i << " | ";
                }
                m_buffer << "policy_mode | policy_flags | policy_num_sample | policy_power_budget\n";
                m_do_header = false;
            }
            m_buffer << telemetry[0].region_id << " | "
                     << geopm_time_diff(&m_time_zero, &(telemetry[0].timestamp)) << " | ";
            for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
                for (int i = 0; i != GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                    m_buffer << it->signal[i] << " | ";
                }
            }
            m_buffer << m_policy.mode << " | "
                     << m_policy.flags << " | "
                     << m_policy.num_sample << " | "
                     << m_policy.power_budget << "\n";

        }
        if (m_buffer.tellp() > m_buffer_limit) {
            m_stream << m_buffer.str();
            m_buffer.str("");
        }
    }

    void Tracer::update(const struct geopm_policy_message_s &policy)
    {
        if (m_is_trace_enabled) {
            m_policy = policy;
        }
    }

    void Tracer::columns(const std::vector<IPlatformIO::m_request_s> &cols)
    {
        if (m_is_trace_enabled) {
            for (auto col : cols) {
                m_column_idx.push_back(m_platform_io.push_signal(col.name,
                                                                 col.domain_type,
                                                                 col.domain_idx));
                m_buffer << pretty_name(col);
                if (m_column_idx.size() != cols.size()) {
                    m_buffer << "|";
                }
            }
            m_buffer << "\n";

            if (!m_stream.good()) {
                throw std::runtime_error("stream not open");
            }
        }
    }

    void Tracer::update(bool is_epoch)
    {
#ifdef GEOPM_DEBUG
        if (m_column_idx.size() == 0) {
            throw Exception("Tracer::update(): No columns added to trace.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double sample = m_platform_io.sample(m_column_idx[0]);
        m_buffer << sample;
        for (size_t col_idx = 1; col_idx < m_column_idx.size(); ++col_idx) {
            sample = m_platform_io.sample(m_column_idx[col_idx]);
            m_buffer << "|" << sample;
        }
        m_buffer << "\n";
    }
    void Tracer::flush(void)
    {
        m_stream << m_buffer.str();
        m_buffer.str("");
        m_stream.close();
        m_is_trace_enabled = false;
    }

    std::string ITracer::pretty_name(const IPlatformIO::m_request_s &col) {
        std::ostringstream result;
        std::string name = col.name;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
        result << name << "-";
        switch(col.domain_type) {
            case IPlatformTopo::M_DOMAIN_BOARD:
                result << "board";
                break;
            case IPlatformTopo::M_DOMAIN_PACKAGE:
                result << "package";
                break;
            case IPlatformTopo::M_DOMAIN_CORE:
                result << "core";
                break;
            case IPlatformTopo::M_DOMAIN_CPU:
                result << "cpu";
                break;
            case IPlatformTopo::M_DOMAIN_BOARD_MEMORY:
                result << "board_memory";
                break;
            case IPlatformTopo::M_DOMAIN_PACKAGE_MEMORY:
                result << "package_memory";
                break;
            case IPlatformTopo::M_DOMAIN_BOARD_NIC:
                result << "board_nic";
                break;
            case IPlatformTopo::M_DOMAIN_PACKAGE_NIC:
                result << "package_nic";
                break;
            case IPlatformTopo::M_DOMAIN_BOARD_ACCELERATOR:
                result << "board_acc";
                break;
            case IPlatformTopo::M_DOMAIN_PACKAGE_ACCELERATOR:
                result << "package_acc";
                break;
            default:
                throw Exception("Tracer::pretty_name(): unrecognized domain_type",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        result << "-" << col.domain_idx;
        return result.str();
    }
}
