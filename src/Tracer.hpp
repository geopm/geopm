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

#ifndef TRACER_HPP_INCLUDE
#define TRACER_HPP_INCLUDE

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <list>

#include "PlatformIO.hpp"

struct geopm_region_info_s;

namespace geopm
{
    /// @brief Abstract base class for the Tracer object defines the interface.
    class Tracer
    {
        public:
            Tracer() = default;
            virtual ~Tracer() = default;
            /// @brief Set up default columns and add columns to be
            //         provided by the Agent.
            virtual void columns(const std::vector<std::string> &agent_cols) = 0;
            /// @brief Update the trace with telemetry samples and
            ///        region info.  The Tracer samples values for
            ///        default and environment columns and the
            ///        remaining signal values are provided by the
            ///        Agent.
            /// @param [in] agent_signals Values for signals provided
            ///        by the agent.
            virtual void update(const std::vector<double> &agent_signals) = 0;
            /// @brief Write the remaining trace data to the file and
            ///        stop tracing.
            virtual void flush(void) = 0;
            /// @brief Returns the column header to be displayed in
            ///        the trace.  The string is based on the signal
            ///        name.  If the domain type is board, the name
            ///        only will be used.  Otherwise, the column
            ///        header includes the name, domain type, and
            ///        domain index.
            /// @param [in] col The request structure containing the
            ///        signal name, domain type, and domain index.
            static std::string pretty_name(const PlatformIO::m_request_s &col);
    };

    class PlatformIO;
    class PlatformTopo;

    /// @brief Class used to write a trace of the telemetry and policy.
    class TracerImp : public Tracer
    {
        public:
            /// @brief TracerImp constructor.
            TracerImp(const std::string &start_time);
            TracerImp(const std::string &start_time,
                      const std::string &file_path,
                      const std::string &hostname,
                      const std::string &agent,
                      const std::string &profile_name,
                      bool do_trace,
                      PlatformIO &platform_io,
                      const PlatformTopo &platform_topo,
                      const std::string &env_column,
                      int precision);
            /// @brief TracerImp destructor, virtual.
            virtual ~TracerImp();
            void columns(const std::vector<std::string> &agent_cols) override;
            void update(const std::vector<double> &agent_signals) override;
            void flush(void) override;
        private:
            void detect_format(const std::string &col_name, int telemetry_idx);
            /// @brief Format and write the values in m_last_telemetry to the trace.
            void write_line(void);
            std::string m_file_path;
            std::string m_header;
            std::string m_hostname;
            bool m_is_trace_enabled;
            std::ofstream m_stream;
            std::ostringstream m_buffer;
            off_t m_buffer_limit;

            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::string m_env_column; // extra columns from environment
            int m_precision;
            std::vector<int> m_column_idx; // columns sampled by TracerImp
            std::set<int> m_hex_format_idx;
            std::set<int> m_region_format_idx;
            std::vector<double> m_last_telemetry;
            int m_region_hash_idx = -1;
            int m_region_hint_idx = -1;
            int m_region_count_idx = -1;
            int m_region_progress_idx = -1;
            int m_region_runtime_idx = -1;
    };
}

#endif
