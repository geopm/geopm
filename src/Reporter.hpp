/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <cstdint>

#include <map>
#include <set>
#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <functional>

namespace geopm
{
    class Comm;
    class ApplicationIO;
    class TreeComm;

    /// @brief A class used by the Controller to format the report at
    ///        the end of a run.  Most of the information for the
    ///        report is passed into the generate() method at the end
    ///        of a run, however the Reporter is also responsible for
    ///        pushing some per-region signals to indicate that they
    ///        should be tracked by the PlatformIO.
    class Reporter
    {
        public:
            Reporter() = default;
            virtual ~Reporter() = default;
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
            /// @param [in] agent_host_report Optional list of
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
                                  const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                  const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                                  const ApplicationIO &application_io,
                                  std::shared_ptr<Comm> comm,
                                  const TreeComm &tree_comm) = 0;
            struct region_info {
                std::string name;
                uint64_t hash;
                double per_rank_avg_runtime;
                    //double network_time;
                int count;
            };
    };

    class PlatformIO;
    class PlatformTopo;
    class SampleAggregator;
    class ProcessRegionAggregator;

    class ReporterImp : public Reporter
    {
        public:
            ReporterImp(const std::string &start_time,
                        const std::string &report_name,
                        PlatformIO &platform_io,
                        const PlatformTopo &platform_topo,
                        int rank);
            ReporterImp(const std::string &start_time,
                        const std::string &report_name,
                        PlatformIO &platform_io,
                        const PlatformTopo &platform_topo,
                        int rank,
                        std::shared_ptr<SampleAggregator> sample_agg,
                        std::shared_ptr<ProcessRegionAggregator> proc_agg,
                        const std::string &env_signal,
                        const std::string &policy_path,
                        bool do_endpoint);
            virtual ~ReporterImp() = default;
            void init(void) override;
            void update(void) override;
            void generate(const std::string &agent_name,
                          const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                          const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                          const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                          const ApplicationIO &application_io,
                          std::shared_ptr<Comm> comm,
                          const TreeComm &tree_comm) override;
        private:
            /// @brief Set up structures used to calculate region-synchronous
            ///        field data to be sampled from SampleAggregator.
            void init_sync_fields(void);
            /// @brief Returns the memoy high water mark for the
            ///        controller process.
            double get_max_memory(void);

            // TODO: yaml functions could be static?
            // TODO: rename to indent_write?  does not enforce correct yaml
            // indent level * 2 spaces per indent
            void yaml_write(std::ostream &os, int indent_level, const std::string &val);
            void yaml_write(std::ostream &os, int indent_level,
                            const std::vector<std::pair<std::string, std::string> > &data);
            void yaml_write(std::ostream &os, int indent_level,
                            const std::vector<std::pair<std::string, double> > &data);

            std::vector<std::pair<std::string, double> > get_region_data(const region_info &region);

            std::string m_start_time;
            std::string m_report_name;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<SampleAggregator> m_sample_agg;
            std::shared_ptr<ProcessRegionAggregator> m_proc_region_agg;
            const std::string m_env_signals;
            const std::string m_policy_path;
            bool m_do_endpoint;
            int m_rank;
            double m_sticker_freq;
            int m_epoch_count_idx;

            // Mapping from pushed signal name to index
            std::map<std::string, int> m_sync_signal_idx;

            // Fields for each section in order.  function can be
            // passthrough, or combo of other fields
            struct m_sync_field_s
            {
                std::string field_label;
                std::vector<std::string> supporting_signals;
                std::function<double(uint64_t, const std::vector<std::string>&)> func;
            };
            // All default fields supported by sample aggregator
            std::vector<m_sync_field_s> m_sync_fields;

            // Signals added through environment
            std::vector<std::pair<std::string, int> > m_env_signal_name_idx;
    };
}

#endif
