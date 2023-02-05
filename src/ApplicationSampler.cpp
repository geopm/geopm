/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <map>
#include <functional>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>

#include "ApplicationSamplerImp.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"
#include "ProfileSampler.hpp"
#include "geopm/Exception.hpp"
#include "RecordFilter.hpp"
#include "Environment.hpp"
#include "ValidateRecord.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/PlatformTopo.hpp"
#include "Scheduler.hpp"
#include "record.hpp"
#include "geopm_debug.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"
#include "geopm_shmem.h"

namespace geopm
{
    ApplicationSampler &ApplicationSampler::application_sampler(void)
    {
        static ApplicationSamplerImp instance;
        return instance;
    }

    std::set<uint64_t> ApplicationSampler::region_hash_network(void)
    {
        static std::set <uint64_t> result = region_hash_network_once();
        return result;
    }

    std::set<uint64_t> ApplicationSampler::region_hash_network_once(void)
    {
        std::set<uint64_t> ret;
        std::set<std::string> network_funcs {"MPI_Allgather",
                                             "MPI_Allgatherv",
                                             "MPI_Allreduce",
                                             "MPI_Alltoall",
                                             "MPI_Alltoallv",
                                             "MPI_Alltoallw",
                                             "MPI_Barrier",
                                             "MPI_Bcast",
                                             "MPI_Bsend",
                                             "MPI_Bsend_init",
                                             "MPI_Gather",
                                             "MPI_Gatherv",
                                             "MPI_Neighbor_allgather",
                                             "MPI_Neighbor_allgatherv",
                                             "MPI_Neighbor_alltoall",
                                             "MPI_Neighbor_alltoallv",
                                             "MPI_Neighbor_alltoallw",
                                             "MPI_Reduce",
                                             "MPI_Reduce_scatter",
                                             "MPI_Reduce_scatter_block",
                                             "MPI_Rsend",
                                             "MPI_Rsend_init",
                                             "MPI_Scan",
                                             "MPI_Scatter",
                                             "MPI_Scatterv",
                                             "MPI_Waitall",
                                             "MPI_Waitany",
                                             "MPI_Wait",
                                             "MPI_Waitsome",
                                             "MPI_Exscan",
                                             "MPI_Recv",
                                             "MPI_Send",
                                             "MPI_Sendrecv",
                                             "MPI_Sendrecv_replace",
                                             "MPI_Ssend"};
        for (auto const &func_name : network_funcs) {
            ret.insert(geopm_crc32_str(func_name.c_str()));
        }
        return ret;
    }

    ApplicationSamplerImp::ApplicationSamplerImp()
        : ApplicationSamplerImp(nullptr,
                                platform_topo(),
                                std::map<int, m_process_s> {},
                                environment().do_record_filter(),
                                environment().record_filter(),
                                {},
                                environment().timeout() != -1)
    {

    }

    ApplicationSamplerImp::ApplicationSamplerImp(std::shared_ptr<ApplicationStatus> status,
                                                 const PlatformTopo &platform_topo,
                                                 const std::map<int, m_process_s> &process_map,
                                                 bool is_filtered,
                                                 const std::string &filter_name,
                                                 const std::vector<bool> &is_cpu_active,
                                                 bool do_profile)
        : m_time_zero(geopm::time_zero())
        , m_status(status)
        , m_topo(platform_topo)
        , m_num_cpu(m_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_process_map(process_map)
        , m_is_filtered(is_filtered)
        , m_filter_name(filter_name)
        , m_hint_time(m_num_cpu, std::array<double, GEOPM_NUM_REGION_HINT>{})
        , m_is_cpu_active(is_cpu_active)
        , m_update_time({{0, 0}})
        , m_is_first_update(true)
        , m_hint_last(m_num_cpu, uint64_t(GEOPM_REGION_HINT_UNSET))
        , m_do_profile(do_profile)
        , m_per_cpu_process(m_num_cpu, -1)
    {
        if (m_is_cpu_active.empty()) {
            m_is_cpu_active.resize(m_num_cpu, false);
        }
    }

    ApplicationSamplerImp::~ApplicationSamplerImp()
    {
        for (auto const& process : m_process_map) {
            if (process.second.record_log_shmem) {
                process.second.record_log_shmem->unlink();
            }
        }
        if (m_status_shmem) {
            m_status_shmem->unlink();
        }
    }

    void ApplicationSamplerImp::time_zero(const geopm_time_s &start_time)
    {
        m_time_zero = start_time;
    }

    void ApplicationSamplerImp::update(const geopm_time_s &curr_time)
    {
        if (!m_do_profile || !m_status || !m_sampler) {
            return;
        }
        // TODO: temporary until handshake fixed
        m_sampler->check_sample_end();

        m_status->update_cache();
        if (m_is_first_update) {
            for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
                m_hint_last[cpu_idx] = m_status->get_hint(cpu_idx);
            }
        }
        else {
            GEOPM_DEBUG_ASSERT((int) m_hint_time.size() == m_num_cpu &&
                               (int) m_hint_last.size() == m_num_cpu,
                               "Mismatch in CPU/hint vectors");
            double time_delta = geopm_time_diff(&m_update_time, &curr_time);
            for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
                m_hint_time[cpu_idx][m_hint_last[cpu_idx]] += time_delta;
                m_hint_last[cpu_idx] = m_status->get_hint(cpu_idx);
            }
        }
        m_is_first_update = false;
        m_update_time = curr_time;
        // Dump the record log from each process, filter the results,
        // and reindex the short region event signals.

        // Clear the buffers that we will be building.
        m_record_buffer.clear();
        m_short_region_buffer.clear();
        // Iterate over the record log for each process
        for (auto &proc_map_it : m_process_map) {
            // Record the location in the record buffer where this
            // process' data begins for updating the short region
            // event signals.
            size_t record_offset = m_record_buffer.size();
            // Get data from the record log
            auto &proc_it = proc_map_it.second;
            proc_it.record_log->dump(proc_it.records, proc_it.short_regions);
            if (m_is_filtered) {
                // Filter and check the records and push them onto
                // m_record_buffer
                for (const auto &record_it : proc_it.records) {
                    for (auto &filtered_it : proc_it.filter->filter(record_it)) {
                        proc_it.valid.check(filtered_it);
                        m_record_buffer.push_back(filtered_it);
                    }
                }
            }
            else {
                // Check the records and push them onto m_record_buffer
                for (const auto &record : proc_it.records) {
                    proc_it.valid.check(record);
                }
                m_record_buffer.insert(m_record_buffer.end(),
                                       proc_it.records.begin(),
                                       proc_it.records.end());
            }
            // Update the "signal" field for all of the short region
            // events to have the right offset.
            size_t short_region_remain = proc_it.short_regions.size();
            for (auto record_it = m_record_buffer.begin() + record_offset;
                 short_region_remain > 0 &&
                 record_it != m_record_buffer.end();
                 ++record_it) {
                if (record_it->event == EVENT_SHORT_REGION) {
                    record_it->signal += m_short_region_buffer.size();
                    --short_region_remain;
                }
            }
            m_short_region_buffer.insert(m_short_region_buffer.end(),
                                         proc_it.short_regions.begin(),
                                         proc_it.short_regions.end());
        }
    }

    std::vector<record_s> ApplicationSamplerImp::get_records(void) const
    {
        return m_record_buffer;
    }

    short_region_s ApplicationSamplerImp::get_short_region(uint64_t event_signal) const
    {
        if (event_signal >= m_short_region_buffer.size()) {
            throw Exception("ApplicationSampler::get_short_region(), event_signal does not match any short region handle",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_short_region_buffer[event_signal];
    }

    std::map<uint64_t, std::string> ApplicationSamplerImp::get_name_map(uint64_t name_key) const
    {
        throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "() is not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return {};
    }

    uint64_t ApplicationSamplerImp::cpu_region_hash(int cpu_idx) const
    {
        if (!m_status) {
            return GEOPM_REGION_HASH_APP;
        }
        return m_status->get_hash(cpu_idx);
    }

    uint64_t ApplicationSamplerImp::cpu_hint(int cpu_idx) const
    {
        if (!m_status) {
            return GEOPM_REGION_HINT_INACTIVE;
        }
        return m_status->get_hint(cpu_idx);
    }

    double ApplicationSamplerImp::cpu_hint_time(int cpu_idx, uint64_t hint) const
    {
        double result = NAN;
        if (cpu_idx < 0 || cpu_idx >= m_num_cpu) {
            throw Exception("ApplicationSampler::" + std::string(__func__) +
                            "(): cpu_idx is out of range: " + std::to_string(cpu_idx),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_cpu_active[cpu_idx]) {
            geopm::check_hint(hint);
            result = m_hint_time[cpu_idx][hint];
        }
        return result;
    }

    double ApplicationSamplerImp::cpu_progress(int cpu_idx) const
    {
        if (!m_status) {
            return 0;
        }
        return m_status->get_progress_cpu(cpu_idx);
    }

    std::vector<int> ApplicationSamplerImp::per_cpu_process(void) const
    {
        return m_per_cpu_process;
    }

    void ApplicationSamplerImp::connect(void)
    {
        if (!m_status && m_do_profile) {
            std::string shmem_path = shmem_path_prof("status", getpid(), geteuid());
            m_status_shmem = SharedMemory::make_unique_user(shmem_path, 0);
            if (m_status_shmem->size() < ApplicationStatus::buffer_size(m_num_cpu)) {
                throw Exception("ApplicationSamplerImp::connect(): Status shared memory buffer is incorrectly sized",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_status = ApplicationStatus::make_unique(m_num_cpu, m_status_shmem);
            GEOPM_DEBUG_ASSERT(m_process_map.empty(),
                               "m_process_map is not empty, but we are connecting");
            // Get list of PIDs that are connected
            auto service_proxy = ServiceProxy::make_unique();
            // TODO: Wait until that all PIDs have connected
            sleep(1);
            std::vector<int> client_pids = service_proxy->platform_get_profile_pids(m_profile_name);
            int num_cpu = geopm_sched_num_cpu();
            cpu_set_t *cpuset = CPU_ALLOC(num_cpu);
            cpu_set_t *all_cpuset = CPU_ALLOC(num_cpu);
            if (cpuset == nullptr || all_cpuset == nullptr) {
                throw Exception("ApplicationSamplerImp::connect(): Failed to allocate cpuset: ",
                                ENOMEM, __FILE__, __LINE__);
            }
            try {
                CPU_ZERO(all_cpuset);
                for (const auto &pid : client_pids) {
                    std::string shmem_path = shmem_path_prof("record-log", pid, geteuid());
                    std::shared_ptr<SharedMemory> record_log_shmem =
                        SharedMemory::make_unique_user(shmem_path, 0);
                    if (recorc_log_shmem->size() < ApplicationRecordLog::buffer_size()) {
                        throw Exception("ApplicationSamplerImp::connect(): "
                                        "Record log shared memory buffer is incorrectly sized",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    auto emplace_ret = m_process_map.emplace(proc_it, m_process_s {});
                    auto &process = emplace_ret.first->second;
                    if (m_is_filtered) {
                        process.filter = RecordFilter::make_unique(m_filter_name);
                    }
                    process.record_log_shmem = record_log_shmem;
                    process.record_log = ApplicationRecordLog::make_unique(record_log_shmem);
                    process.records.reserve(ApplicationRecordLog::max_record());
                    process.short_regions.reserve(ApplicationRecordLog::max_region());
                    int err = geopm_sched_proc_cpuset_pid(pid, num_cpu, cpuset);
                    if (err) {
                        throw Exception("Failed to get cpuset for pid: " + std::to_string(pid),
                                        err, __FILE__, __LINE__);
                    }
                    CPU_OR(all_cpuset, all_cpuset, cpuset);
                    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
                        if (CPU_ISSET(cpu_idx, cpuset)) {
                            process.cpus.push_back(cpu_idx);
                            // TODO: Last PID wins if the affinity masks overlap
                            m_per_cpu_process.at(cpu_idx) = pid;
                        }
                    }
                }
                std::set<int> inactive_cpu;
                for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
                    if (!CPU_ISSET(cpu_idx, all_cpuset)) {
                        inactive_cpu.insert(cpu_idx);
                    }
                }
                m_status->set_valid_cpu(inactive_cpu, false);
                m_status->update_cache();
            }
            except (...) {
                if (all_cpuset) {
                    CPU_FREE(all_cpuset);
                }
                if (cpuset) {
                    CPU_FREE(cpuset);
                }
                throw;
            }
            CPU_FREE(all_cpuset);
            CPU_FREE(cpuset);
        }
        // Try to pin the sampling thread to a free core
        std::set<int> sampler_cpu_set = {sampler_cpu()};
        auto sampler_cpu_mask = make_cpu_set(m_num_cpu, sampler_cpu_set);

        int err = sched_setaffinity(0, CPU_ALLOC_SIZE(m_num_cpu), sampler_cpu_mask.get());
        if (err) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> Unable to affinitize sampling thread to CPU "
                      << *(sampler_cpu_set.begin())
                      << ", sched_setaffinity() failed: " << strerror(errno) << "\n";
#endif
        }
    }

    int ApplicationSamplerImp::sampler_cpu(void)
    {
        int result = m_num_cpu - 1;
        int num_core = m_topo.num_domain(GEOPM_DOMAIN_CORE);
        bool found_inactive_core = false;
        bool found_inactive_cpu = false;
        std::vector<bool> is_core_active(num_core, false);
        for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
            if (m_is_cpu_active[cpu_idx]) {
                is_core_active.at(m_topo.domain_idx(GEOPM_DOMAIN_CORE, cpu_idx)) = true;
            }
        }
        for (int core_idx = num_core - 1; core_idx != -1; --core_idx) {
            if (!is_core_active.at(core_idx)) {
                std::set<int> inactive_cpu = m_topo.domain_nested(GEOPM_DOMAIN_CPU,
                                                                  GEOPM_DOMAIN_CORE,
                                                                  core_idx);
                GEOPM_DEBUG_ASSERT(inactive_cpu.size() != 0,
                                   "Valid core index returned no nested CPUs");
                result = *(inactive_cpu.rbegin());
                found_inactive_core = true;
                found_inactive_cpu = true;
                break;
            }
        }
        if (!found_inactive_core) {
            for (int cpu_idx = m_num_cpu - 1; cpu_idx != -1; --cpu_idx) {
                if(!m_is_cpu_active[cpu_idx]) {
                    result = cpu_idx;
                    found_inactive_cpu = true;
                    break;
                }
            }
        }
#ifdef GEOPM_DEBUG
        std::cout << "Info: <geopm> ApplicationSampler::sampler_cpu(): The Controller will run on logical CPU " << result << std::endl;
#endif

        if (!found_inactive_core) {
            std::cerr << "Warning: <geopm> ApplicationSampler::sampler_cpu(): User requested "
                      << "all cores for application.  GEOPM will share a core with the "
                      << "Application, running on logical CPU " << result;

            if (result == 0) {
                std::cerr << ", where the OS will run system threads.";
            }

            std::cerr << "." << std::endl;
        }

        if (!found_inactive_cpu) {
            std::cerr << "Warning: <geopm> ApplicationSampler::sampler_cpu(): "
                      << "GEOPM will run on the same HW thread (oversubscribe) as the "
                      << "Application." << std::endl;
        }

#ifdef GEOPM_DEBUG
        if (found_inactive_core && 0 == m_topo.domain_idx(GEOPM_DOMAIN_CORE, result)) {
            std::cerr << "Warning: <geopm> ApplicationSampler::sampler_cpu(): User requested "
                      << "all cores except core 0 for the application.  GEOPM will share a "
                      << "core with the OS, running on logical CPU " << result << "."
                      << std::endl;
        }
#endif

        return result;
    }

    void ApplicationSamplerImp::set_sampler(std::shared_ptr<ProfileSampler> sampler)
    {
        m_sampler = sampler;
    }

    std::shared_ptr<ProfileSampler> ApplicationSamplerImp::get_sampler(void)
    {
        return m_sampler;
    }
}
