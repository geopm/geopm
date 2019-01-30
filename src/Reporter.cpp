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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sstream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <limits.h>

#include "Reporter.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "RegionAggregator.hpp"
#include "ApplicationIO.hpp"
#include "Comm.hpp"
#include "TreeComm.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "OMPT.hpp"
#include "geopm.h"
#include "geopm_hash.h"
#include "geopm_version.h"
#include "config.h"

#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

namespace geopm
{
    Reporter::Reporter(const std::string &start_time, const std::string &report_name, IPlatformIO &platform_io, int rank)
        : Reporter(start_time, report_name, platform_io, rank,
                   std::unique_ptr<IRegionAggregator>(new RegionAggregator))
    {

    }

    Reporter::Reporter(const std::string &start_time, const std::string &report_name, IPlatformIO &platform_io, int rank,
                       std::unique_ptr<IRegionAggregator> agg)
        : m_start_time(start_time)
        , m_report_name(report_name)
        , m_platform_io(platform_io)
        , m_region_agg(std::move(agg))
        , m_rank(rank)
    {

    }

    void Reporter::init(void)
    {
        m_region_bulk_runtime_idx = m_region_agg->push_signal_total("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_energy_pkg_idx = m_region_agg->push_signal_total("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_energy_dram_idx = m_region_agg->push_signal_total("ENERGY_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_clk_core_idx = m_region_agg->push_signal_total("CYCLES_THREAD", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_clk_ref_idx = m_region_agg->push_signal_total("CYCLES_REFERENCE", IPlatformTopo::M_DOMAIN_BOARD, 0);

        if (!m_rank) {
            // check if report file can be created
            if (!m_report_name.empty()) {
                std::ofstream test_open(m_report_name);
                if (!test_open.good()) {
                    std::cerr << "Warning: unable to open report file '" << m_report_name
                              << "' for writing: " << strerror(errno) << std::endl;
                }
                std::remove(m_report_name.c_str());
            }
        }
        m_region_agg->init();
    }

    void Reporter::update()
    {
        m_region_agg->read_batch();
    }

    void Reporter::generate(const std::string &agent_name,
                            const std::vector<std::pair<std::string, std::string> > &agent_report_header,
                            const std::vector<std::pair<std::string, std::string> > &agent_node_report,
                            const std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > &agent_region_report,
                            const IApplicationIO &application_io,
                            std::shared_ptr<Comm> comm,
                            const ITreeComm &tree_comm)
    {
        std::string report_name(application_io.report_name());
        if (report_name.size() == 0) {
            return;
        }

        int rank = comm->rank();
        std::ofstream master_report;
        if (!rank) {
            master_report.open(report_name);
            if (!master_report.good()) {
                throw Exception("Failed to open report file", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            // make header
            master_report << "##### geopm " << geopm_version() << " #####" << std::endl;
            master_report << "Start Time: " << m_start_time << std::endl;
            master_report << "Profile: " << application_io.profile_name() << std::endl;
            master_report << "Agent: " << agent_name << std::endl;
            for (const auto &kv : agent_report_header) {
                master_report << kv.first << ": " << kv.second << std::endl;
            }
        }
        // per-node report
        std::ostringstream report;
        report << "\nHost: " << hostname() << std::endl;
        for (const auto &kv : agent_node_report) {
            report << kv.first << ": " << kv.second << std::endl;
        }
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
            std::string region_name = region;
            ompt_pretty_name(region_name);

            int count = application_io.total_count(region_hash);
            if (count > 0) {
                region_ordered.push_back({region_name,
                                          region_hash,
                                          application_io.total_region_runtime(region_hash),
                                          count});
            }
        }
        // sort based on averge runtime, descending
        std::sort(region_ordered.begin(), region_ordered.end(),
                  [] (const region_info &a,
                      const region_info &b) -> bool {
                      return a.per_rank_avg_runtime >= b.per_rank_avg_runtime;
                  });
        // Add unmarked and epoch at the end
        // Note here we map the private region id notion of
        // GEOPM_REGION_ID_UNMARKED to pubilc GEOPM_REGION_HASH_UNMARKED.
        region_ordered.push_back({"unmarked-region",
                                  GEOPM_REGION_HASH_UNMARKED,
                                  application_io.total_region_runtime(GEOPM_REGION_ID_UNMARKED),
                                  0});
        // Total epoch runtime for report includes MPI time and
        // ignore time, but they are removed from the runtime returned
        // by the API.  This behavior is to support the EPOCH_RUNTIME
        // signal used by the balancer, but will be changed in the future.
        region_ordered.push_back({"epoch",
                                  GEOPM_REGION_HASH_EPOCH,
                                  application_io.total_epoch_runtime(),
                                  /// @todo epoch_count?
                                  application_io.total_count(GEOPM_REGION_ID_EPOCH)});

        for (const auto &region : region_ordered) {
            if (GEOPM_REGION_HASH_EPOCH != region.hash) {
#ifdef GEOPM_DEBUG
                if (GEOPM_REGION_HASH_INVALID == region.hash) {
                    throw Exception("Reporter::generate(): Invalid hash value detected.",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
#endif
                report << "Region " << region.name << " (0x" << std::hex
                       << std::setfill('0') << std::setw(16)
                       << region.hash << std::dec << "):"
                       << std::setfill('\0') << std::setw(0)
                       << std::endl;
            }
            else {
                report << "Epoch Totals:"
                       << std::endl;
            }
            report << "    runtime (sec): " << region.per_rank_avg_runtime << std::endl;
            report << "    sync-runtime (sec): " << m_region_agg->sample_total(m_region_bulk_runtime_idx, region.hash) << std::endl;
            report << "    package-energy (joules): " << m_region_agg->sample_total(m_energy_pkg_idx, region.hash) << std::endl;
            report << "    dram-energy (joules): " << m_region_agg->sample_total(m_energy_dram_idx, region.hash) << std::endl;
            double numer = m_region_agg->sample_total(m_clk_core_idx, region.hash);
            double denom = m_region_agg->sample_total(m_clk_ref_idx, region.hash);
            double freq = denom != 0 ? 100.0 * numer / denom : 0.0;
            report << "    frequency (%): " << freq << std::endl;
            report << "    frequency (Hz): " << freq / 100.0 * m_platform_io.read_signal("CPUINFO::FREQ_STICKER", IPlatformTopo::M_DOMAIN_BOARD, 0) << std::endl;
            /// @todo total_epoch_runtime_mpi
            report << "    mpi-runtime (sec): " << application_io.total_region_runtime_mpi(region.hash != GEOPM_REGION_HASH_EPOCH ? region.hash : GEOPM_REGION_ID_EPOCH) << std::endl;
            report << "    count: " << region.count << std::endl;
            const auto &it = agent_region_report.find(region.hash);
                if (it != agent_region_report.end()) {
                    for (const auto &kv : agent_region_report.at(region.hash)) {
                        report << "    " << kv.first << ": " << kv.second << std::endl;
                    }
                }
        }
        // extra runtimes for epoch region
        report << "    epoch-runtime-ignore (sec): " << application_io.total_epoch_runtime_ignore() << std::endl;

        double total_runtime = application_io.total_app_runtime();
        report << "Application Totals:" << std::endl
               << "    runtime (sec): " << total_runtime << std::endl
               << "    package-energy (joules): " << application_io.total_app_energy_pkg() << std::endl
               << "    dram-energy (joules): " << application_io.total_app_energy_dram() << std::endl
               << "    mpi-runtime (sec): " << application_io.total_app_runtime_mpi() << std::endl
               << "    ignore-time (sec): " << application_io.total_app_runtime_ignore() << std::endl;

        std::string max_memory = get_max_memory();
        report << "    geopmctl memory HWM: " << max_memory << std::endl;
        report << "    geopmctl network BW (B/sec): " << tree_comm.overhead_send() / total_runtime << std::endl;

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
            master_report << report_buffer.data();
            master_report << std::endl;
            master_report.close();
        }
    }

    std::string Reporter::get_max_memory()
    {
        char status_buffer[8192];
        status_buffer[8191] = '\0';
        const char *proc_path = "/proc/self/status";

        int fd = open(proc_path, O_RDONLY);
        if (fd == -1) {
            throw Exception("Reporter::generate(): Unable to open " + std::string(proc_path),
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        ssize_t num_read = read(fd, status_buffer, 8191);
        if (num_read == -1) {
            (void)close(fd);
            throw Exception("Reporter::generate(): Unable to read " + std::string(proc_path),
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        status_buffer[num_read] = '\0';

        int err = close(fd);
        if (err) {
            throw Exception("Reporter::generate(): Unable to close " + std::string(proc_path),
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
            throw Exception("Controller::generate_report(): Unable to get memory overhead from /proc",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return max_memory;
    }
}
