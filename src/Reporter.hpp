/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            /// @brief Handle any initialization that must take place
            ///        after the Controller has connected to the
            ///        application.
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
            virtual std::string generate(const std::string &profile_name,
                                         const std::string &agent_name,
                                         const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                                         const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                         const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report) = 0;
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
                        const std::vector<std::pair<std::string, int> > &env_signal,
                        const std::string &policy_path,
                        bool do_endpoint,
                        bool do_profile);
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
            std::string generate(const std::string &profile_name,
                                 const std::string &agent_name,
                                 const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                                 const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                 const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report) override;

        private:
            /// @brief number of spaces for each indentation
            static constexpr int M_SPACES_INDENT = 2;
            // Number of levels of indentation for each section of the report
            static constexpr int M_INDENT_HEADER = 0;
            static constexpr int M_INDENT_HOST = 0;
            static constexpr int M_INDENT_HOST_NAME = M_INDENT_HOST + 1;
            static constexpr int M_INDENT_HOST_AGENT = M_INDENT_HOST_NAME + 1;
            static constexpr int M_INDENT_REGION = M_INDENT_HOST_NAME + 1;
            static constexpr int M_INDENT_REGION_FIELD = M_INDENT_REGION + 1;
            static constexpr int M_INDENT_UNMARKED = M_INDENT_HOST_NAME + 1;
            static constexpr int M_INDENT_UNMARKED_FIELD = M_INDENT_UNMARKED + 1;
            static constexpr int M_INDENT_EPOCH = M_INDENT_HOST_NAME + 1;
            static constexpr int M_INDENT_EPOCH_FIELD = M_INDENT_EPOCH + 1;
            static constexpr int M_INDENT_TOTALS = M_INDENT_HOST_NAME + 1;
            static constexpr int M_INDENT_TOTALS_FIELD = M_INDENT_TOTALS + 1;
            /// @brief Set up structures used to calculate region-synchronous
            ///        field data to be sampled from SampleAggregator.
            void init_sync_fields(void);
            /// @brief Set up signals added by the user through the environment.
            void init_environment_signals(void);
            /// @brief Samples values for fields common to all
            ///        regions, epoch data, and application totals.
            ///        The vector returned by this method is intended
            ///        to be passed to yaml_write().
            std::vector<std::pair<std::string, double> > get_region_data(uint64_t region_hash);
            /// @brief Returns the memoy high water mark for the
            ///        controller process.
            double get_max_memory(void);
            static void yaml_write(std::ostream &os, int indent_level,
                                   const std::string &val);
            static void yaml_write(std::ostream &os, int indent_level,
                                   const std::vector<std::pair<std::string, std::string> > &data);
            static void yaml_write(std::ostream &os, int indent_level,
                                   const std::vector<std::pair<std::string, double> > &data);

            std::string create_header(const std::string &agent_name,
                                      const std::string &profile_name,
                                      const std::vector<std::pair<std::string, std::string> > &agent_report_header);
            std::string create_report(const std::set<std::string> &region_name_set, double max_memory, double comm_overhead,
                                      const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                      const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report);
            std::string gather_report(const std::string &host_report, std::shared_ptr<Comm> comm);

            std::string m_start_time;
            std::string m_report_name;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            std::shared_ptr<SampleAggregator> m_sample_agg;
            std::shared_ptr<ProcessRegionAggregator> m_proc_region_agg;
            const std::vector<std::pair<std::string, int> > m_env_signals;
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
            bool m_do_profile;
    };
}

#endif
