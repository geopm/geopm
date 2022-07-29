/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TRACER_HPP_INCLUDE
#define TRACER_HPP_INCLUDE

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <list>
#include <memory>

#include "geopm/PlatformIO.hpp"
#include "CSV.hpp"

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
            virtual void columns(const std::vector<std::string> &agent_cols,
                                 const std::vector<std::function<std::string(double)> > &agent_formats) = 0;
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
    };

    class PlatformIO;
    class PlatformTopo;
    class CSV;

    /// @brief Class used to write a trace of the telemetry and policy.
    class TracerImp : public Tracer
    {
        public:
            /// @brief TracerImp constructor.
            TracerImp(const std::string &start_time);
            TracerImp(const std::string &start_time,
                      const std::string &file_path,
                      const std::string &hostname,
                      bool do_trace,
                      PlatformIO &platform_io,
                      const PlatformTopo &platform_topo,
                      const std::vector<std::pair<std::string, int> > &env_column);
            /// @brief TracerImp destructor, virtual.
            virtual ~TracerImp() = default;
            void columns(const std::vector<std::string> &agent_cols,
                         const std::vector<std::function<std::string(double)> > &agent_formats) override;
            void update(const std::vector<double> &agent_signals) override;
            void flush(void) override;
        private:
            struct m_request_s {
                std::string name;
                int domain_type;
                int domain_idx;
                std::function<std::string(double)> format;
            };

            std::vector<std::string> env_signals(void);
            std::vector<int> env_domains(void);
            std::vector<std::function<std::string(double)> > env_formats(void);

            std::string m_file_path;
            std::string m_header;
            std::string m_hostname;
            bool m_is_trace_enabled;

            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::vector<std::pair<std::string, int> > m_env_column; // extra columns from environment
            std::vector<int> m_column_idx; // columns sampled by TracerImp
            std::vector<double> m_last_telemetry;
            const size_t M_BUFFER_SIZE;
            std::unique_ptr<CSV> m_csv;
            int m_region_hash_idx;
            int m_region_hint_idx;
            int m_region_progress_idx;
            int m_region_runtime_idx;
            int m_num_base_columns;
    };
}

#endif
