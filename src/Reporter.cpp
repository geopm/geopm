/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"

#include "Reporter.hpp"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

#include <sstream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "SampleAggregator.hpp"
#include "ProcessRegionAggregator.hpp"
#include "ApplicationIO.hpp"
#include "Comm.hpp"
#include "TreeComm.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm.h"
#include "geopm_hash.h"
#include "geopm_version.h"
#include "Environment.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    ReporterImp::ReporterImp(const std::string &start_time,
                             const std::string &report_name,
                             PlatformIO &platform_io,
                             const PlatformTopo &platform_topo,
                             int rank)
        : ReporterImp(start_time,
                      report_name,
                      platform_io,
                      platform_topo,
                      rank,
                      SampleAggregator::make_unique(),
                      nullptr,
                      environment().report_signals(),
                      environment().policy(),
                      environment().do_endpoint())
    {

    }

    ReporterImp::ReporterImp(const std::string &start_time,
                             const std::string &report_name,
                             PlatformIO &platform_io,
                             const PlatformTopo &platform_topo,
                             int rank,
                             std::shared_ptr<SampleAggregator> sample_agg,
                             std::shared_ptr<ProcessRegionAggregator> proc_agg,
                             const std::string &env_signals,
                             const std::string &policy_path,
                             bool do_endpoint)
        : m_start_time(start_time)
        , m_report_name(report_name)
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_sample_agg(sample_agg)
        , m_proc_region_agg(proc_agg)
        , m_env_signals(env_signals)
        , m_policy_path(policy_path)
        , m_do_endpoint(do_endpoint)
        , m_rank(rank)
        , m_sticker_freq(m_platform_io.read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0))
        , m_epoch_count_idx(-1)
    {
        GEOPM_DEBUG_ASSERT(m_sample_agg != nullptr, "m_sample_agg cannot be null");

        init_sync_fields();

        init_environment_signals();

        m_epoch_count_idx = m_platform_io.push_signal("EPOCH_COUNT", GEOPM_DOMAIN_BOARD, 0);

        if (!m_rank) {
            // check if report file can be created
            if (!m_report_name.empty()) {
                std::ofstream test_open(m_report_name);
                if (!test_open.good()) {
                    std::cerr << "Warning: <geopm> Unable to open report file '" << m_report_name
                              << "' for writing: " << strerror(errno) << std::endl;
                }
                std::remove(m_report_name.c_str());
            }
        }
    }

    void ReporterImp::init(void)
    {
        if (m_proc_region_agg == nullptr) {
            // ProcessRegionAggregator should not be constructed until
            // application connection is established.
            m_proc_region_agg = ProcessRegionAggregator::make_unique();
        }
    }

    void ReporterImp::update()
    {
        m_sample_agg->update();
        m_proc_region_agg->update();
    }

    void ReporterImp::generate(const std::string &agent_name,
                               const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                               const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                               const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                               const ApplicationIO &application_io,
                               std::shared_ptr<Comm> comm,
                               const TreeComm &tree_comm)
    {
        std::string report_name(application_io.report_name());
        if (report_name.size() == 0) {
            return;
        }

        int rank = comm->rank();
        std::ofstream common_report;
        if (!rank) {
            common_report.open(report_name);
            if (!common_report.good()) {
                throw Exception("Failed to open report file", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            // make header
            std::string policy_str = "{}";
            if (m_do_endpoint) {
                policy_str = "DYNAMIC";
            }
            else if (m_policy_path.size() > 0) {
                try {
                    policy_str = read_file(m_policy_path);
                }
                catch(...) {
                    policy_str = m_policy_path;
                }
            }
            std::vector<std::pair<std::string, std::string> > header {
                {"GEOPM Version", geopm_version()},
                {"Start Time", m_start_time},
                {"Profile", application_io.profile_name()},
                {"Agent", agent_name},
                {"Policy", policy_str}
            };
            yaml_write(common_report, M_INDENT_HEADER, header);
            yaml_write(common_report, M_INDENT_HEADER, agent_report_header);
            common_report << "\n";
            yaml_write(common_report, M_INDENT_HOST, "Hosts:");
        }

        // per-host report
        std::ostringstream report;
        yaml_write(report, M_INDENT_HOST_NAME, hostname() + ":");
        yaml_write(report, M_INDENT_HOST_AGENT, agent_host_report);
        yaml_write(report, M_INDENT_REGION, "Regions:");

        // vector of region data, in descending order by runtime
        struct region_info {
            std::string name;
            uint64_t hash;
            double per_rank_avg_runtime;
            int count;
        };

        std::vector<region_info> region_ordered;
        auto region_name_set = application_io.region_name_set();
        for (const auto &region : region_name_set) {
            uint64_t region_hash = geopm_crc32_str(region.c_str());
            int count = m_proc_region_agg->get_count_average(region_hash);
            if (count > 0) {
                region_ordered.push_back({region,
                                          region_hash,
                                          m_proc_region_agg->get_runtime_average(region_hash),
                                          count});
            }
        }
        // sort based on averge runtime, descending
        std::sort(region_ordered.begin(), region_ordered.end(),
                  [] (const region_info &a,
                      const region_info &b) -> bool {
                      return a.per_rank_avg_runtime > b.per_rank_avg_runtime;
                  });

        double total_marked_runtime = 0.0;
        for (const auto &region : region_ordered) {
#ifdef GEOPM_DEBUG
            if (GEOPM_REGION_HASH_INVALID == region.hash) {
                throw Exception("ReporterImp::generate(): Invalid hash value detected.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            yaml_write(report, M_INDENT_REGION, "-");
            yaml_write(report, M_INDENT_REGION_FIELD,
                       {{"region", '"' + region.name + '"'},
                        {"hash", geopm::string_format_hex(region.hash)}});
            yaml_write(report, M_INDENT_REGION_FIELD,
                       {{"runtime (s)", region.per_rank_avg_runtime},
                        {"count", region.count}});
            auto region_data = get_region_data(region.hash);
            yaml_write(report, M_INDENT_REGION_FIELD, region_data);
            const auto &it = agent_region_report.find(region.hash);
            if (it != agent_region_report.end()) {
                yaml_write(report, M_INDENT_REGION_FIELD, agent_region_report.at(region.hash));
            }
            total_marked_runtime += region.per_rank_avg_runtime;
        }

        yaml_write(report, M_INDENT_UNMARKED, "Unmarked Totals:");
        double unmarked_time = m_sample_agg->sample_application(m_sync_signal_idx["TIME"]) -
                               total_marked_runtime;
        yaml_write(report, M_INDENT_UNMARKED_FIELD,
                   {{"runtime (s)", unmarked_time},
                    {"count", 0}});
        auto unmarked_data = get_region_data(GEOPM_REGION_HASH_UNMARKED);
        yaml_write(report, M_INDENT_UNMARKED_FIELD, unmarked_data);
        // agent extensions for unmarked
        const auto &it = agent_region_report.find(GEOPM_REGION_HASH_UNMARKED);
        if (it != agent_region_report.end()) {
            yaml_write(report, M_INDENT_UNMARKED_FIELD, agent_region_report.at(GEOPM_REGION_HASH_UNMARKED));
        }

        yaml_write(report, M_INDENT_EPOCH, "Epoch Totals:");
        double epoch_runtime = m_sample_agg->sample_epoch(m_sync_signal_idx["TIME"]);
        int epoch_count = m_platform_io.sample(m_epoch_count_idx);
        yaml_write(report, M_INDENT_EPOCH_FIELD,
                   {{"runtime (s)", epoch_runtime},
                    {"count", epoch_count}});
        auto epoch_data = get_region_data(GEOPM_REGION_HASH_EPOCH);
        yaml_write(report, M_INDENT_EPOCH_FIELD, epoch_data);

        yaml_write(report, M_INDENT_TOTALS, "Application Totals:");
        double total_runtime = m_sample_agg->sample_application(m_sync_signal_idx["TIME"]);
        yaml_write(report, M_INDENT_TOTALS_FIELD,
                   {{"runtime (s)", total_runtime},
                    {"count", 0}});
        auto region_data = get_region_data(GEOPM_REGION_HASH_APP);
        yaml_write(report, M_INDENT_TOTALS_FIELD, region_data);

        // Controller overhead
        std::vector<std::pair<std::string, double> > overhead {
            {"geopmctl memory HWM (B)", get_max_memory()},
            {"geopmctl network BW (B/s)", tree_comm.overhead_send() / total_runtime}
        };
        yaml_write(report, M_INDENT_TOTALS_FIELD, overhead);

        // aggregate reports from every node
        report.seekp(0, std::ios::end);
        size_t buffer_size = (size_t) report.tellp();
        report.seekp(0, std::ios::beg);
        std::vector<char> report_buffer;
        std::vector<size_t> buffer_size_array;
        std::vector<off_t> buffer_displacement;
        int num_ranks = comm->num_rank();
        buffer_size_array.resize(num_ranks);
        buffer_displacement.resize(num_ranks);
        comm->gather(&buffer_size, sizeof(size_t), buffer_size_array.data(),
                     sizeof(size_t), 0);

        if (!rank) {
            int full_report_size = std::accumulate(buffer_size_array.begin(), buffer_size_array.end(), 0) + 1;
            report_buffer.resize(full_report_size);
            buffer_displacement[0] = 0;
            for (int i = 1; i < num_ranks; ++i) {
                buffer_displacement[i] = buffer_displacement[i-1] + buffer_size_array[i-1];
            }
        }

        comm->gatherv((void *) (report.str().data()), sizeof(char) * buffer_size,
                      (void *) report_buffer.data(), buffer_size_array, buffer_displacement, 0);

        if (!rank) {
            report_buffer.back() = '\0';
            common_report << report_buffer.data();
            common_report << std::endl;
            common_report.close();
        }
    }

    void ReporterImp::init_sync_fields(void)
    {
        auto sample_only = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 1, "Wrong number of signals for sample_only()");
            return m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
        };
        auto divide = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 2, "Wrong number of signals for divide()");
            double numer = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denom = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denom == 0 ? 0.0 : numer / denom;
        };
        auto divide_pct = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 2, "Wrong number of signals for divide_pct()");
            double numer = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denom = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denom == 0 ? 0.0 : 100.0 * numer / denom;
        };
        auto divide_sticker_scale = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 2, "Wrong number of signals for divide_sticker_scale()");
            double numer = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denom = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denom == 0 ? 0.0 : m_sticker_freq * numer / denom;
        };

        m_sync_fields = {
            {"sync-runtime (s)", {"TIME"}, sample_only},
            {"package-energy (J)", {"ENERGY_PACKAGE"}, sample_only},
            {"dram-energy (J)", {"ENERGY_DRAM"}, sample_only},
            {"power (W)", {"ENERGY_PACKAGE", "TIME"}, divide},
            {"frequency (%)", {"CYCLES_THREAD", "CYCLES_REFERENCE"}, divide_pct},
            {"frequency (Hz)", {"CYCLES_THREAD", "CYCLES_REFERENCE"}, divide_sticker_scale},
            {"time-hint-network (s)", {"TIME_HINT_NETWORK"}, sample_only},
            {"time-hint-ignore (s)", {"TIME_HINT_IGNORE"}, sample_only},
            {"time-hint-compute (s)", {"TIME_HINT_COMPUTE"}, sample_only},
            {"time-hint-memory (s)", {"TIME_HINT_MEMORY"}, sample_only},
            {"time-hint-io (s)", {"TIME_HINT_IO"}, sample_only},
            {"time-hint-serial (s)", {"TIME_HINT_SERIAL"}, sample_only},
            {"time-hint-parallel (s)", {"TIME_HINT_PARALLEL"}, sample_only},
            {"time-hint-unknown (s)", {"TIME_HINT_UNKNOWN"}, sample_only},
            {"time-hint-unset (s)", {"TIME_HINT_UNSET"}, sample_only},
        };

        for (const auto &field : m_sync_fields) {
            for (const auto &signal : field.supporting_signals) {
                m_sync_signal_idx[signal] = m_sample_agg->push_signal(signal, GEOPM_DOMAIN_BOARD, 0);
            }
        }
    }

    void ReporterImp::init_environment_signals(void)
    {
        for (const std::string &signal_name : string_split(m_env_signals, ",")) {
            std::vector<std::string> signal_name_domain = string_split(signal_name, "@");
            if (signal_name_domain.size() == 2) {
                int domain_type = m_platform_topo.domain_name_to_type(signal_name_domain[1]);
                for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(domain_type); ++domain_idx) {
                    m_env_signal_name_idx.emplace_back(
                        signal_name + '-' + std::to_string(domain_idx),
                        m_sample_agg->push_signal(signal_name_domain[0], domain_type, domain_idx));
                }
            }
            else if (signal_name_domain.size() == 1) {
                m_env_signal_name_idx.emplace_back(
                    signal_name,
                    m_sample_agg->push_signal(signal_name, GEOPM_DOMAIN_BOARD, 0));
            }
            else {
                throw Exception("ReporterImp::init(): Environment report extension contains signals with multiple \"@\" characters.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    std::vector<std::pair<std::string, double> > ReporterImp::get_region_data(uint64_t region_hash)
    {
        std::vector<std::pair<std::string, double> > result;

        // sync fields as initialized in init_sync_fields
        for (auto &field : m_sync_fields) {
            result.push_back({field.field_label, field.func(region_hash, field.supporting_signals)});
        }

        // signals added by user through environment
        for (const auto &env_it : m_env_signal_name_idx) {
            result.push_back({env_it.first, m_sample_agg->sample_region(env_it.second, region_hash)});
        }
        return result;
    }

    double ReporterImp::get_max_memory()
    {
        char status_buffer[8192];
        status_buffer[8191] = '\0';
        const char *proc_path = "/proc/self/status";

        int fd = open(proc_path, O_RDONLY);
        if (fd == -1) {
            throw Exception("ReporterImp::get_max_memory(): Unable to open " + std::string(proc_path),
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        ssize_t num_read = read(fd, status_buffer, 8191);
        if (num_read == -1) {
            (void)close(fd);
            throw Exception("ReporterImp::get_max_memory(): Unable to read " + std::string(proc_path),
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        status_buffer[num_read] = '\0';

        int err = close(fd);
        if (err) {
            throw Exception("ReporterImp::get_max_memory(): Unable to close " + std::string(proc_path),
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::istringstream proc_stream(status_buffer);
        std::string line;
        std::string max_memory;
        const std::string key("VmHWM:");
        while (proc_stream.good()) {
            getline(proc_stream, line);
            if (line.find(key) == 0) {
                max_memory = line.substr(key.length());
                size_t off = max_memory.find_first_not_of(" \t");
                if (off != std::string::npos) {
                    max_memory = max_memory.substr(off);
                }
            }
        }
        if (!max_memory.size()) {
            throw Exception("ReporterImp::get_max_memory(): Unable to get memory overhead from /proc",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        // expect kibibyte units
        size_t len = max_memory.size();
        if (max_memory.substr(len - 2) != "kB") {
            throw Exception("ReporterImp::get_max_memory(): HWM not in units of kB",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double max_memory_bytes = std::stod(max_memory.substr(0, len - 2));
        return max_memory_bytes;
    }

    void ReporterImp::yaml_write(std::ostream &os, int indent_level,
                                 const std::string &val)
    {
        std::string indent(indent_level * M_SPACES_INDENT, ' ');
        os << indent << val << std::endl;
    }

    void ReporterImp::yaml_write(std::ostream &os, int indent_level,
                                 const std::vector<std::pair<std::string, std::string> > &data)
    {
        std::string indent(indent_level * M_SPACES_INDENT, ' ');
        for (const auto &kv : data) {
            os << indent << kv.first << ": " << kv.second << std::endl;
        }
    }

    void ReporterImp::yaml_write(std::ostream &os, int indent_level,
                                 const std::vector<std::pair<std::string, double> > &data)
    {
        std::string indent(indent_level * M_SPACES_INDENT, ' ');
        for (auto kv: data) {
            os << indent << kv.first << ": " << kv.second << std::endl;
        }
    }

}
