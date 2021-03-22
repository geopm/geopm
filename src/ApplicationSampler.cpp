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

#include "config.h"

#include <sched.h>
#include <map>
#include <functional>

#include "ApplicationSamplerImp.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"
#include "ProfileSampler.hpp"
#include "Exception.hpp"
#include "RecordFilter.hpp"
#include "Environment.hpp"
#include "ValidateRecord.hpp"
#include "SharedMemory.hpp"
#include "PlatformTopo.hpp"
#include "record.hpp"
#include "geopm_debug.hpp"
#include "geopm.h"
#include "geopm_hash.h"

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
                                             "MPI_Bcast"
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
                                             "MPI_Rsend"
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
                                platform_topo().num_domain(GEOPM_DOMAIN_CPU),
                                std::map<int, m_process_s> {},
                                environment().do_record_filter(),
                                environment().record_filter(),
                                {})
    {

    }

    const std::map<uint64_t, double> ApplicationSamplerImp::m_hint_time_init = {
        {GEOPM_REGION_HINT_UNSET, 0.0},
        {GEOPM_REGION_HINT_UNKNOWN, 0.0},
        {GEOPM_REGION_HINT_COMPUTE, 0.0},
        {GEOPM_REGION_HINT_MEMORY, 0.0},
        {GEOPM_REGION_HINT_NETWORK, 0.0},
        {GEOPM_REGION_HINT_IO, 0.0},
        {GEOPM_REGION_HINT_SERIAL, 0.0},
        {GEOPM_REGION_HINT_PARALLEL, 0.0},
        {GEOPM_REGION_HINT_IGNORE, 0.0},
        {GEOPM_REGION_HINT_INACTIVE, 0.0},
    };


    ApplicationSamplerImp::ApplicationSamplerImp(std::shared_ptr<ApplicationStatus> status,
                                                 int num_cpu,
                                                 const std::map<int, m_process_s> &process_map,
                                                 bool is_filtered,
                                                 const std::string &filter_name,
                                                 const std::vector<bool> &is_cpu_active)
        : m_time_zero(geopm::time_zero())
        , m_status(status)
        , m_num_cpu(num_cpu)
        , m_process_map(process_map)
        , m_is_filtered(is_filtered)
        , m_filter_name(filter_name)
        , m_hint_time(m_num_cpu, m_hint_time_init)
        , m_is_cpu_active(is_cpu_active)
        , m_update_time({{0, 0}})
        , m_is_first_update(true)
        , m_hint_last(m_num_cpu, GEOPM_REGION_HINT_UNSET)
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
        if (!m_status) {
            throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "(): cannot read process info before connect().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
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
            double time_delta = geopm_time_diff(&m_update_time, &curr_time);
            for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
                m_hint_time[cpu_idx].at(m_hint_last[cpu_idx]) += time_delta;
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
            throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "(): cannot read process info before connect().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_status->get_hash(cpu_idx);
    }

    uint64_t ApplicationSamplerImp::cpu_hint(int cpu_idx) const
    {
        if (!m_status) {
            throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "(): cannot read process info before connect().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
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
            auto &hint_map = m_hint_time[cpu_idx];
            auto it = hint_map.find(hint);
            if (it == hint_map.end()) {
                throw Exception("ApplicationSampler::" + std::string(__func__) +
                                "(): hint is invalid: " + std::to_string(hint),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = it->second;
        }
        return result;
    }

    double ApplicationSamplerImp::cpu_progress(int cpu_idx) const
    {
        if (!m_status) {
            throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "(): cannot read process info before connect().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_status->get_progress_cpu(cpu_idx);
    }

    std::vector<int> ApplicationSamplerImp::per_cpu_process(void) const
    {
        return m_sampler->cpu_rank();
        /// @todo code below will work *after* the handshake is complete
        if (!m_status) {
            throw Exception("ApplicationSamplerImp::" + std::string(__func__) + "(): cannot read process info before connect().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::vector<int> result(m_num_cpu);
        for (int cpu_idx = 0; cpu_idx != m_num_cpu; ++cpu_idx) {
            result[cpu_idx] = m_status->get_process(cpu_idx);
        }
        return result;
    }

    void ApplicationSamplerImp::connect(const std::string &shm_key)
    {
        if (!m_status) {
            std::string shmem_name = shm_key + "-status";
            m_status_shmem = SharedMemory::make_unique_owner(shmem_name,
                                                             ApplicationStatus::buffer_size(m_num_cpu));
            m_status = ApplicationStatus::make_unique(m_num_cpu, m_status_shmem);
            GEOPM_DEBUG_ASSERT(m_process_map.empty(),
                               "m_process_map is not empty, but we are connecting");
            // Convert per-cpu process to a set of the unique process id's
            int cpu_idx = 0;
            std::set<int> proc_set;
            for (const auto &proc_it : per_cpu_process()) {
                if (proc_it != -1) {
                    proc_set.insert(proc_it);
                    m_is_cpu_active.at(cpu_idx) = true;
                }
                ++cpu_idx;
            }
            // For each unique process id create a record log and
            // insert it into map indexed by process id
            for (const auto &proc_it : proc_set) {
                std::string shmem_name = shm_key + "-record-log-" + std::to_string(proc_it);
                std::shared_ptr<SharedMemory> record_log_shmem =
                    SharedMemory::make_unique_owner(shmem_name,
                                                    ApplicationRecordLog::buffer_size());
                auto emplace_ret = m_process_map.emplace(proc_it, m_process_s {});
                auto &process = emplace_ret.first->second;
                if (m_is_filtered) {
                    process.filter = RecordFilter::make_unique(m_filter_name);
                }
                process.record_log_shmem = record_log_shmem;
                process.record_log = ApplicationRecordLog::make_unique(record_log_shmem);
                process.records.reserve(ApplicationRecordLog::max_record());
                process.short_regions.reserve(ApplicationRecordLog::max_region());
            }
        }
        // Try to pin the sampling thread to a free core
        std::vector<bool> cpu_enabled(m_num_cpu, false);
        cpu_enabled.at(sampler_cpu()) = true;
        std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> > cpu_mask =
            make_cpu_set(cpu_enabled);
        (void)sched_setaffinity(0, cpu_set_size(m_num_cpu), cpu_mask.get());
    }

    int ApplicationSamplerImp::sampler_cpu(void)
    {
        int result = 0;
        for (int cpu_idx = m_num_cpu - 1; cpu_idx != -1; --cpu_idx) {
            if (!m_is_cpu_active[cpu_idx]) {
                result = cpu_idx;
                break;
            }
        }
        return result;
    }

    std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> > ApplicationSamplerImp::make_cpu_set(std::vector<bool> cpu_enabled)
    {
        std::unique_ptr<cpu_set_t, std::function<void(cpu_set_t *)> > result(
            CPU_ALLOC(cpu_enabled.size()),
            [](cpu_set_t *ptr)
            {
                CPU_FREE(ptr);
            });
        for (size_t cpu_idx = 0; cpu_idx != cpu_enabled.size(); ++cpu_idx) {
            if (cpu_enabled[cpu_idx]) {
                CPU_SET(cpu_idx, result.get());
            }
            else {
                CPU_CLR(cpu_idx, result.get());
            }
        }
        return result;
    }

    size_t ApplicationSamplerImp::cpu_set_size(int num_cpu)
    {
        return CPU_ALLOC_SIZE(num_cpu);
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
