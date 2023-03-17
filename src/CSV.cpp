/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <climits>
#include <cinttypes>

#include "geopm_version.h"
#include "geopm_hash.h"
#include "geopm/Helper.hpp"
#include "CSV.hpp"
#include "geopm/Exception.hpp"
#include "Environment.hpp"

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
#ifdef GEOPM_ENABLE_MPI
        if (host_name.size()) {
            m_file_path += "-" + host_name;
        }
#endif
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
