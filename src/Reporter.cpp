/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Reporter.hpp"
#include "geopm_reporter.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <cmath>

#include <sstream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "SampleAggregator.hpp"
#include "ProcessRegionAggregator.hpp"
#include "ApplicationIO.hpp"
#include "Comm.hpp"
#include "TreeComm.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_hash.h"
#include "geopm_version.h"
#include "geopm_debug.hpp"
#include "Environment.hpp"
#include "PlatformIOProf.hpp"
#include "geopm_time.h"

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
                      environment().do_endpoint(),
                      environment().timeout() != -1)
    {

    }

    ReporterImp::ReporterImp(const std::string &start_time,
                             const std::string &report_name,
                             PlatformIO &platform_io,
                             const PlatformTopo &platform_topo,
                             int rank,
                             std::shared_ptr<SampleAggregator> sample_agg,
                             std::shared_ptr<ProcessRegionAggregator> proc_agg,
                             const std::vector<std::pair<std::string, int> > &env_signals,
                             const std::string &policy_path,
                             bool do_endpoint,
                             bool do_profile)
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
        , m_do_profile(do_profile)
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
        if (m_do_profile && m_proc_region_agg == nullptr) {
            // ProcessRegionAggregator should not be constructed until
            // application connection is established.
            m_proc_region_agg = ProcessRegionAggregator::make_unique();
        }
    }

    void ReporterImp::update()
    {
        m_sample_agg->update();
        if (m_do_profile) {
            m_proc_region_agg->update();
        }
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
            common_report << create_header(agent_name, application_io.profile_name(), agent_report_header);
        }

        std::string host_report = create_report(application_io.region_name_set(),
                                                get_max_memory(),
                                                tree_comm.overhead_send(),
                                                agent_host_report,
                                                agent_region_report);
        std::string full_report = gather_report(host_report, comm);

        if (!rank) {
            common_report << full_report;
            common_report << std::endl;
            common_report.close();
        }
    }

    std::string ReporterImp::generate(const std::string &profile_name,
                                      const std::string &agent_name,
                                      const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                                      const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                      const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report)
    {
        std::ostringstream common_report;
        common_report << create_header(agent_name, profile_name, agent_report_header);

        common_report << create_report({},
                                       get_max_memory(),
                                       0.0,
                                       agent_host_report,
                                       agent_region_report);
        common_report << std::endl;
        return common_report.str();
    }

    std::string ReporterImp::create_header(const std::string &agent_name,
                                           const std::string &profile_name,
                                           const std::vector<std::pair<std::string, std::string> > &agent_report_header)
    {
        std::ostringstream common_report;
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
            {"Profile", profile_name},
            {"Agent", agent_name},
            {"Policy", policy_str}
        };
        yaml_write(common_report, M_INDENT_HEADER, header);
        yaml_write(common_report, M_INDENT_HEADER, agent_report_header);
        common_report << "\n";
        yaml_write(common_report, M_INDENT_HOST, "Hosts:");
        return common_report.str();
    }


    std::string ReporterImp::create_report(const std::set<std::string> &region_name_set, double max_memory, double comm_overhead,
                                           const std::vector<std::pair<std::string, std::string> > &agent_host_report,
                                           const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report)
    {
        // per-host report
        std::ostringstream report;
        yaml_write(report, M_INDENT_HOST_NAME, hostname() + ":");
        yaml_write(report, M_INDENT_HOST_AGENT, agent_host_report);
        if (region_name_set.size() != 0) {
            yaml_write(report, M_INDENT_REGION, "Regions:");
        }

        // vector of region data, in descending order by runtime
        struct region_info {
            std::string name;
            uint64_t hash;
            double per_rank_avg_runtime;
            int count;
        };

        std::vector<region_info> region_ordered;
        GEOPM_DEBUG_ASSERT(region_name_set.size() == 0 ||
                           m_proc_region_agg != nullptr,
                           "ReporterImp::create_report(): region set is not empty, but region aggregator pointer is null");
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

        double epoch_count = m_platform_io.sample(m_epoch_count_idx);
        // Do not add epoch or unmarked section if no application attached
        if (!std::isnan(epoch_count)) {
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
            yaml_write(report, M_INDENT_EPOCH_FIELD,
                       {{"runtime (s)", epoch_runtime},
                        {"count", (int)epoch_count}});
            auto epoch_data = get_region_data(GEOPM_REGION_HASH_EPOCH);
            yaml_write(report, M_INDENT_EPOCH_FIELD, epoch_data);
        }

        yaml_write(report, M_INDENT_TOTALS, "Application Totals:");
        double total_runtime = m_sample_agg->sample_application(m_sync_signal_idx["TIME"]);
        yaml_write(report, M_INDENT_TOTALS_FIELD,
                   {{"runtime (s)", total_runtime},
                    {"count", 0}});
        auto region_data = get_region_data(GEOPM_REGION_HASH_APP);
        yaml_write(report, M_INDENT_TOTALS_FIELD, region_data);
        // Controller overhead
        std::vector<std::pair<std::string, double> > overhead {
            {"geopmctl memory HWM (B)", max_memory},
            {"geopmctl network BW (B/s)", comm_overhead / total_runtime}
        };
        yaml_write(report, M_INDENT_TOTALS_FIELD, overhead);
        return report.str();
    }

    std::string ReporterImp::gather_report(const std::string &host_report, std::shared_ptr<Comm> comm)
    {
        // aggregate reports from every node
        size_t buffer_size = host_report.size();
        std::vector<char> report_buffer;
        std::vector<size_t> buffer_size_array;
        std::vector<off_t> buffer_displacement;
        int num_ranks = comm->num_rank();
        buffer_size_array.resize(num_ranks);
        buffer_displacement.resize(num_ranks);
        comm->gather(&buffer_size, sizeof(size_t), buffer_size_array.data(),
                     sizeof(size_t), 0);

        if (comm->rank() == 0) {
            int full_report_size = std::accumulate(buffer_size_array.begin(), buffer_size_array.end(), 0) + 1;
            report_buffer.resize(full_report_size);
            buffer_displacement[0] = 0;
            for (int i = 1; i < num_ranks; ++i) {
                buffer_displacement[i] = buffer_displacement[i-1] + buffer_size_array[i-1];
            }
        }

        comm->gatherv((void *) (host_report.c_str()), sizeof(char) * buffer_size,
                      (void *) report_buffer.data(), buffer_size_array, buffer_displacement, 0);
        if (report_buffer.size() != 0) {
            report_buffer.back() = '\0';
        }
        else {
            report_buffer.push_back('\0');
        }
        return report_buffer.data();
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
            double numerator = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denominator = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denominator == 0 ? 0.0 : numerator / denominator;
        };
        auto divide_pct = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 2, "Wrong number of signals for divide_pct()");
            double numerator = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denominator = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denominator == 0 ? 0.0 : 100.0 * numerator / denominator;
        };
        auto divide_sticker_scale = [this](uint64_t hash, const std::vector<std::string> &sig) -> double
        {
            GEOPM_DEBUG_ASSERT(sig.size() == 2, "Wrong number of signals for divide_sticker_scale()");
            double numerator = m_sample_agg->sample_region(m_sync_signal_idx[sig[0]], hash);
            double denominator = m_sample_agg->sample_region(m_sync_signal_idx[sig[1]], hash);
            return denominator == 0 ? 0.0 : m_sticker_freq * numerator / denominator;
        };

        m_sync_fields = {
            {"sync-runtime (s)", {"TIME"}, sample_only},
            {"package-energy (J)", {"CPU_ENERGY"}, sample_only},
            {"dram-energy (J)", {"DRAM_ENERGY"}, sample_only},
            {"power (W)", {"CPU_ENERGY", "TIME"}, divide},
            {"frequency (%)", {"CPU_CYCLES_THREAD", "CPU_CYCLES_REFERENCE"}, divide_pct},
            {"frequency (Hz)", {"CPU_CYCLES_THREAD", "CPU_CYCLES_REFERENCE"}, divide_sticker_scale},
            {"time-hint-network (s)", {"TIME_HINT_NETWORK"}, sample_only},
            {"time-hint-ignore (s)", {"TIME_HINT_IGNORE"}, sample_only},
            {"time-hint-compute (s)", {"TIME_HINT_COMPUTE"}, sample_only},
            {"time-hint-memory (s)", {"TIME_HINT_MEMORY"}, sample_only},
            {"time-hint-io (s)", {"TIME_HINT_IO"}, sample_only},
            {"time-hint-serial (s)", {"TIME_HINT_SERIAL"}, sample_only},
            {"time-hint-parallel (s)", {"TIME_HINT_PARALLEL"}, sample_only},
            {"time-hint-unknown (s)", {"TIME_HINT_UNKNOWN"}, sample_only},
            {"time-hint-unset (s)", {"TIME_HINT_UNSET"}, sample_only},
            {"time-hint-spin (s)", {"TIME_HINT_SPIN"}, sample_only},
        };

        auto all_names = m_platform_io.signal_names();
        std::vector<m_sync_field_s> conditional_sync_fields = {
            {"gpu-energy (J)", {"GPU_ENERGY"}, sample_only},
            {"gpu-power (W)", {"GPU_POWER"}, sample_only},
            {"gpu-core-energy (J)", {"GPU_CORE_ENERGY"}, sample_only},
            {"gpu-core-power (W)", {"GPU_CORE_POWER"}, sample_only},
            {"gpu-frequency (Hz)", {"GPU_CORE_FREQUENCY_STATUS"}, sample_only},
            {"uncore-frequency (Hz)", {"CPU_UNCORE_FREQUENCY_STATUS"}, sample_only}
        };

        for (const auto &field : conditional_sync_fields) {
            for (const auto &signal : field.supporting_signals) {
                if (all_names.count(signal) != 0) {
                    m_sync_fields.push_back(field);
                }
            }
        }

        for (const auto &field : m_sync_fields) {
            for (const auto &signal : field.supporting_signals) {
                m_sync_signal_idx[signal] = m_sample_agg->push_signal(signal, GEOPM_DOMAIN_BOARD, 0);
            }
        }
    }

    void ReporterImp::init_environment_signals(void)
    {
        for (const auto& signal_domain_pair : m_env_signals) {
            if (signal_domain_pair.second == GEOPM_DOMAIN_BOARD) {
                m_env_signal_name_idx.emplace_back(
                    signal_domain_pair.first,
                    m_sample_agg->push_signal(signal_domain_pair.first, GEOPM_DOMAIN_BOARD, 0));
            }
            else {
                std::string full_signal_name;
                const int& domain_type = signal_domain_pair.second;
                const int num_domains = m_platform_topo.num_domain(domain_type);
                for (int domain_idx = 0; domain_idx < num_domains; ++domain_idx) {
                    full_signal_name = signal_domain_pair.first + "@" + m_platform_topo.domain_type_to_name(signal_domain_pair.second);
                    full_signal_name += '-';
                    full_signal_name += std::to_string(domain_idx);
                    m_env_signal_name_idx.emplace_back(
                        full_signal_name,
                        m_sample_agg->push_signal(signal_domain_pair.first, domain_type, domain_idx));
                }
            }
        }
    }

    std::vector<std::pair<std::string, double> > ReporterImp::get_region_data(uint64_t region_hash)
    {
        std::vector<std::pair<std::string, double> > result;

        // sync fields as initialized in init_sync_fields
        for (const auto &field : m_sync_fields) {
            double value = field.func(region_hash, field.supporting_signals);
            if (!std::isnan(value)) { // Remove nan fields
                result.push_back({field.field_label, value});
            }
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
        for (const auto &kv: data) {
            os << indent << kv.first << ": " << kv.second << std::endl;
        }
    }

    static Reporter &basic_reporter(const std::string &start_time)
    {
        static ReporterImp instance(start_time,
                                    "",
                                    PlatformIOProf::platform_io(),
                                    platform_topo(),
                                    0);
        return instance;
    }

    static Reporter &basic_reporter(void)
    {
        return basic_reporter("");
    }

}

int geopm_reporter_init(void)
{
    char start_time[NAME_MAX];
    int err = geopm_time_string(NAME_MAX, start_time);
    if (!err) {
        try {
            geopm::basic_reporter(start_time);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
            err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
        }
    }
    return err;
}

int geopm_reporter_update(void)
{
    int err = 0;
    try {
        geopm::basic_reporter().update();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_reporter_generate(const char *profile_name,
                            const char *agent_name,
                            size_t result_max,
                            char *result)
{
    int err = 0;
    try {
        std::string result_cxx = geopm::basic_reporter().generate(profile_name, agent_name, {}, {}, {});
        result[result_max - 1] = '\0';
        strncpy(result, result_cxx.c_str(), result_max);
        if (result[result_max - 1] != '\0') {
            err = GEOPM_ERROR_INVALID;
            result[result_max - 1] = '\0';
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}
