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
#include <climits>
#include <cinttypes>

#include "geopm_version.h"
#include "geopm_hash.h"
#include "Helper.hpp"
#include "CSV.hpp"
#include "Exception.hpp"
#include "Environment.hpp"
#include "config.h"

namespace geopm
{
    CSVImp::CSVImp(const std::string &file_path,
                   const std::string &host_name,
                   const std::string &start_time,
                   size_t buffer_size)
        : M_NAME_FORMAT_MAP {{"double", string_format_double},
                             {"float", string_format_float},
                             {"integer", string_format_integer},
                             {"hex", string_format_hex},
                             {"raw64", string_format_raw64}}
        , M_SEPARATOR('|')
        , m_file_path(file_path)
        , m_buffer_limit(buffer_size)
        , m_is_active(false)
    {
        if (host_name.size()) {
            m_file_path += "-" + host_name;
        }
        m_stream.open(m_file_path);
        if (!m_stream.good()) {
            throw Exception("Unable to open CSV file '" + m_file_path + "'",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        write_header(start_time, host_name);
    }

    CSVImp::~CSVImp()
    {
        flush();
    }

    void CSVImp::add_column(const std::string &name)
    {
        add_column(name, "double");
    }

    void CSVImp::add_column(const std::string &name, const std::string &format)
    {
        if (m_is_active) {
            throw Exception("CSVImp::add_column() cannot be called after activate()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const auto &it = M_NAME_FORMAT_MAP.find(format);
        if (M_NAME_FORMAT_MAP.end() == it) {
            throw Exception("CSVImp::add_column(), format is unknown: " + format,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_column_name.push_back(name);
        m_column_format.push_back(it->second);
    }

    void CSVImp::add_column(const std::string &name, std::function<std::string(double)> format)
    {
        if (m_is_active) {
            throw Exception("CSVImp::add_column() cannot be called after activate()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_column_name.push_back(name);
        m_column_format.push_back(format);
    }

    void CSVImp::update(const std::vector<double> &sample)
    {
        if (!m_is_active) {
            throw Exception("CSVImp::activate() must be called prior to update",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (sample.size() != m_column_format.size()) {
            throw Exception("CSVImp::update(): Input vector incorrectly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        for (size_t sample_idx = 0; sample_idx != sample.size(); ++sample_idx) {
            if (sample_idx) {
                m_buffer << M_SEPARATOR;
            }
            m_buffer << m_column_format[sample_idx](sample[sample_idx]);
        }
        m_buffer << "\n";

        // if buffer is full, flush to file
        if (m_buffer.tellp() > m_buffer_limit) {
            flush();
        }
    }

    void CSVImp::flush(void)
    {
        m_stream << m_buffer.str();
        m_stream.flush();
        m_buffer.str("");
    }

    void CSVImp::write_header(const std::string &start_time, const std::string &host_name)
    {
        m_buffer << "# geopm_version: " << geopm_version() << "\n"
                 << "# start_time: " << start_time << "\n"
                 << "# profile_name: " << environment().profile() << "\n"
                 << "# node_name: " << host_name << "\n"
                 << "# agent: " << environment().agent() << "\n";
    }

    void CSVImp::activate(void)
    {
        if (m_is_active == false) {
            m_is_active = true;
            write_names();
        }
    }

    void CSVImp::write_names(void)
    {
        bool is_once = true;
        for (const auto &it : m_column_name) {
            if (is_once) {
               is_once = false;
            }
            else {
                m_buffer << M_SEPARATOR;
            }
            m_buffer << it;
        }
        m_buffer << '\n';
    }
}
