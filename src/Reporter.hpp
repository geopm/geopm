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

#ifndef REPORTER_HPP_INCLUDE
#define REPORTER_HPP_INCLUDE

#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <memory>
#include <vector>

namespace geopm
{
    class Comm;
    class IApplicationIO;
    class ITreeComm;

    /// @brief A class used by the Controller to format the report at
    ///        the end of a run.  Most of the information for the
    ///        report is passed into the generate() method at the end
    ///        of a run, however the Reporter is also responsible for
    ///        pushing some per-region signals to indicate that they
    ///        should be tracked by the PlatformIO.
    class IReporter
    {
        public:
            IReporter() = default;
            virtual ~IReporter() = default;
            /// @brief Set up per-region tracking of energy signals
            ///        and signals used to calculate frequency.
            virtual void init(void) = 0;
            /// @brief Read values from PlatformIO to update
            ///        aggregated samples.
            virtual void update(void) = 0;
            /// @brief Create a report for this node.  If the node is
            ///        the root controller, format the header,
            ///        aggregate all other node reports, and write the
            ///        report to the file indicated in the
            ///        environment.
            /// @param [in] agent_name Name of the Agent.
            /// @param [in] agent_report_header Optional list of
            ///             key-value pairs from the agent to be added
            ///             to the report header.
            /// @param [in] agent_node_report Optional list of
            ///             key-value pairs from the agent to be added
            ///             to the host section of the report.
            /// @param [in] agent_region_report Optional mapping from
            ///             region ID to lists of key-value pairs from
            ///             the agent to be added as additional
            ///             information about each region.
            /// @param [in] application_io Reference to the
            ///             ApplicationIO owned by the controller.
            /// @param [in] comm Shared pointer to the Comm owned by
            ///             the controller.
            /// @param [in] tree_comm Reference to the TreeComm owned
            ///             by the controller.
            virtual void generate(const std::string &agent_name,
                                  const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                                  const std::vector<std::pair<std::string, std::string> > &agent_node_report,
                                  const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                                  const IApplicationIO &application_io,
                                  std::shared_ptr<Comm> comm,
                                  const ITreeComm &tree_comm) = 0;
    };

    class IPlatformIO;
    class IPlatformTopo;
    class IRegionAggregator;

    class Reporter : public IReporter
    {
        public:
            Reporter(const std::string &start_time, const std::string &report_name, IPlatformIO &platform_io, IPlatformTopo &platform_topo, int rank);
            Reporter(const std::string &start_time, const std::string &report_name, IPlatformIO &platform_io, IPlatformTopo &platform_topo, int rank,
                     std::unique_ptr<IRegionAggregator> agg);
            virtual ~Reporter() = default;
            void init(void) override;
            void update(void) override;
            void generate(const std::string &agent_name,
                          const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                          const std::vector<std::pair<std::string, std::string> > &agent_node_report,
                          const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                          const IApplicationIO &application_io,
                          std::shared_ptr<Comm> comm,
                          const ITreeComm &tree_comm) override;
        private:
            std::string get_max_memory(void);

            std::string m_start_time;
            std::string m_report_name;
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            std::unique_ptr<IRegionAggregator> m_region_agg;
            int m_rank;
            int m_region_bulk_runtime_idx;
            int m_energy_pkg_idx;
            int m_energy_dram_idx;
            int m_clk_core_idx;
            int m_clk_ref_idx;
            std::vector<std::string> m_env_signal_name;
            std::vector<int> m_env_signal_idx;
    };
}

#endif
