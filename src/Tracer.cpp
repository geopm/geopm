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

#include "Tracer.hpp"

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <time.h>

#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "Environment.hpp"
#include "geopm_hash.h"
#include "geopm_version.h"
#include "geopm.h"
#include "geopm_internal.h"
#include "config.h"

namespace geopm
{
    TracerImp::TracerImp(const std::string &start_time)
        : TracerImp(start_time, environment().trace(), hostname(),
                    environment().do_trace(), platform_io(), platform_topo(),
                    environment().trace_signals())
    {

    }

    TracerImp::TracerImp(const std::string &start_time,
                         const std::string &file_path,
                         const std::string &hostname,
                         bool do_trace,
                         PlatformIO &platform_io,
                         const PlatformTopo &platform_topo,
                         const std::string &env_column)
        : m_is_trace_enabled(do_trace)
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_env_column(env_column)
        , M_BUFFER_SIZE(134217728) // 128 MiB
    {
        if (m_is_trace_enabled) {
            m_csv = make_unique<CSVImp>(file_path, hostname, start_time, M_BUFFER_SIZE);
        }
    }

    void TracerImp::columns(const std::vector<std::string> &agent_cols,
                            const std::vector<std::function<std::string(double)> > &col_formats)
    {
        if (m_is_trace_enabled) {
            if (col_formats.size() != 0 &&
                col_formats.size() != agent_cols.size()) {
                throw Exception("TracerImp::columns(): input vectors not of equal size",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            // default columns
            std::vector<struct m_request_s> base_columns({
                    {"TIME", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("TIME")},
                    {"EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("EPOCH_COUNT")},
                    {"REGION_HASH", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("REGION_HASH")},
                    {"REGION_HINT", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("REGION_HINT")},
                    {"REGION_PROGRESS", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("REGION_PROGRESS")},
                    {"REGION_COUNT", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("REGION_COUNT")},
                    {"REGION_RUNTIME", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("REGION_RUNTIME")},
                    {"ENERGY_PACKAGE", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("ENERGY_PACKAGE")},
                    {"ENERGY_DRAM", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("ENERGY_DRAM")},
                    {"POWER_PACKAGE", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("POWER_PACKAGE")},
                    {"POWER_DRAM", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("POWER_DRAM")},
                    {"FREQUENCY", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("FREQUENCY")},
                    {"CYCLES_THREAD", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("CYCLES_THREAD")},
                    {"CYCLES_REFERENCE", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("CYCLES_REFERENCE")},
                    {"TEMPERATURE_CORE", GEOPM_DOMAIN_BOARD, 0,
                     m_platform_io.format_function("TEMPERATURE_CORE")}});

            m_region_hash_idx = 2;
            m_region_hint_idx = 3;
            m_region_progress_idx = 4;
            m_region_runtime_idx = 6;

            // extra columns from environment
            std::vector<std::string> env_sig = env_signals();
            std::vector<int> env_dom = env_domains();
            std::vector<std::function<std::string(double)> > env_form = env_formats();
#ifdef GEOPM_DEBUG
            if (env_sig.size() != env_dom.size() ||
                env_sig.size() != env_form.size()) {
                throw Exception("Tracer::columns(): private functions returned different size vectors",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            size_t num_sig = env_sig.size();
            for (size_t sig_idx = 0; sig_idx != num_sig; ++sig_idx) {
                int num_dom = m_platform_topo.num_domain(env_dom.at(sig_idx));
                for (int dom_idx = 0; dom_idx != num_dom; ++dom_idx) {
                    base_columns.push_back({env_sig.at(sig_idx), env_dom.at(sig_idx), dom_idx, env_form.at(sig_idx)});
                }
            }
            // set up columns to be sampled by TracerImp
            for (const auto &col : base_columns) {
                m_column_idx.push_back(m_platform_io.push_signal(col.name,
                                                                 col.domain_type,
                                                                 col.domain_idx));
                std::string column_name = col.name;
                if (col.domain_type != GEOPM_DOMAIN_BOARD) {
                    column_name += "-" + PlatformTopo::domain_type_to_name(col.domain_type);
                    column_name += "-" + std::to_string(col.domain_idx);
                }
                m_csv->add_column(column_name, col.format);
            }
            // columns from agent; will be sampled by agent
            size_t num_col = agent_cols.size();
            for (size_t col_idx = 0; col_idx != num_col; ++col_idx) {
                std::function<std::string(double)> format = col_formats.size() ? col_formats.at(col_idx) : string_format_double;
                m_csv->add_column(agent_cols.at(col_idx), format);
            }
            m_csv->activate();
            m_last_telemetry.resize(base_columns.size() + num_col);
        }
    }

    void TracerImp::update(const std::vector<double> &agent_values,
                           std::list<geopm_region_info_s> region_entry_exit)
    {
        if (m_is_trace_enabled) {
#ifdef GEOPM_DEBUG
            if (m_column_idx.size() == 0) {
                throw Exception("TracerImp::update(): No columns added to trace.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
            if (m_column_idx.size() + agent_values.size() != m_last_telemetry.size()) {
                throw Exception("TracerImp::update(): Last telemetry buffer not sized correctly.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            // save values to be reused for region entry/exit
            size_t col_idx = 0;
            for (; col_idx < m_column_idx.size(); ++col_idx) {
                m_last_telemetry[col_idx] = m_platform_io.sample(m_column_idx[col_idx]);
            }
            for (const auto &val : agent_values) {
                m_last_telemetry[col_idx] = val;
                ++col_idx;
            }
#ifdef GEOPM_TRACE_BLOAT
            // save region id and progress, which will get written over by entry/exit
            double region_hash = m_last_telemetry[m_region_hash_idx];
            double region_hint = m_last_telemetry[m_region_hint_idx];
            double region_progress = m_last_telemetry[m_region_progress_idx];
            double region_runtime = m_last_telemetry[m_region_runtime_idx];

            // insert samples for region entry/exit
            size_t idx = 0;
            for (const auto &reg : region_entry_exit) {
                // skip the last region entry if it matches the
                // sampled telemetry region hash, hint, and progress
                if (!((idx == region_entry_exit.size() - 1) &&
                      region_progress == reg.progress &&
                      region_progress == 0.0 &&
                      region_hash == reg.hash &&
                      region_hint == reg.hint)) {
                    m_last_telemetry[m_region_hash_idx] = reg.hash;
                    m_last_telemetry[m_region_hint_idx] = reg.hint;
                    m_last_telemetry[m_region_progress_idx] = reg.progress;
                    m_last_telemetry[m_region_runtime_idx] = reg.runtime;
                    /// @todo There are no updates to the region count field.
                    ///       Rather than fix this issue, will just remove
                    ///       these inserted rows in future commit.
                    m_csv->update(m_last_telemetry);
                }
                ++idx;
            }
            // print sampled data last
            m_last_telemetry[m_region_hash_idx] = region_hash;
            m_last_telemetry[m_region_hint_idx] = region_hint;
            m_last_telemetry[m_region_progress_idx] = region_progress;
            m_last_telemetry[m_region_runtime_idx] = region_runtime;
#endif // GEOPM_TRACE_BLOAT
            m_csv->update(m_last_telemetry);
        }
    }

    void TracerImp::flush(void)
    {
        if (m_is_trace_enabled) {
            m_csv->flush();
        }
    }

    std::vector<std::string> TracerImp::env_signals(void)
    {
        std::vector<std::string> result;
        for (const auto &extra_signal : string_split(m_env_column, ",")) {
            std::vector<std::string> signal_domain = string_split(extra_signal, "@");
            result.push_back(signal_domain[0]);
        }
        return result;
    }

    std::vector<int> TracerImp::env_domains(void)
    {
        std::vector<int> result;
        for (const auto &extra_signal : string_split(m_env_column, ",")) {
            std::vector<std::string> signal_domain = string_split(extra_signal, "@");
            if (signal_domain.size() == 2) {
                result.push_back(PlatformTopo::domain_name_to_type(signal_domain[1]));
            }
            else if (signal_domain.size() == 1) {
                result.push_back(GEOPM_DOMAIN_BOARD);
            }
            else {
                throw Exception("TracerImp::columns(): Environment trace extension contains signals with multiple \"@\" characters.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return result;
    }

    std::vector<std::function<std::string(double)> > TracerImp::env_formats(void)
    {
        std::vector<std::function<std::string(double)> > result;
        std::vector<std::string> signals = env_signals();
        for (const auto &it : env_signals()) {
            result.push_back(m_platform_io.format_function(it));
        }
        return result;
    }
}
