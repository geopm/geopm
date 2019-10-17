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
#include "Profile.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <float.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <algorithm>
#include <iostream>

#include "geopm.h"
#include "geopm_internal.h"
#include "geopm_time.h"
#include "geopm_sched.h"
#include "Environment.hpp"
#include "PlatformTopo.hpp"
#include "ProfileTable.hpp"
#include "ProfileThread.hpp"
#include "SampleScheduler.hpp"
#include "ControlMessage.hpp"
#include "SharedMemoryImp.hpp"
#include "Exception.hpp"
#include "Comm.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    ProfileImp::ProfileImp(const std::string &prof_name, const std::string &key_base,
                           const std::string &report, double timeout, bool do_region_barrier,
                           std::unique_ptr<Comm> comm, std::unique_ptr<ControlMessage> ctl_msg,
                           const PlatformTopo &topo, std::unique_ptr<ProfileTable> table,
                           std::shared_ptr<ProfileThreadTable> t_table,
                           std::unique_ptr<SampleScheduler> scheduler,
                           std::shared_ptr<Comm> reduce_comm)
        : m_is_enabled(true)
        , m_prof_name(prof_name)
        , m_report(report)
        , m_timeout(timeout)
        , m_do_region_barrier(do_region_barrier)
        , m_curr_region_id(0)
        , m_num_enter(0)
        , m_progress(0.0)
        , m_ctl_shmem(nullptr)
        , m_ctl_msg(std::move(ctl_msg))
        , m_table_shmem(nullptr)
        , m_table(std::move(table))
        , m_tprof_shmem(nullptr)
        , m_tprof_table(t_table)
        , m_scheduler(std::move(scheduler))
        , m_shm_comm(nullptr)
        , m_rank(0)
        , m_shm_rank(0)
        , m_parent_region(0)
        , m_parent_progress(0.0)
        , m_parent_num_enter(0)
        , m_reduce_comm(reduce_comm)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
    {
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
        if (m_reduce_comm == nullptr) {
            m_reduce_comm = geopm::comm_factory().make_plugin(environment().comm());
        }
#else
        /// read and write to satisfy clang ifndef GEOPM_OVERHEAD
        ++m_overhead_time;
        --m_overhead_time;
        ++m_overhead_time_startup;
        --m_overhead_time_startup;
        ++m_overhead_time_shutdown;
        --m_overhead_time_shutdown;
#endif
        std::string sample_key(key_base + "-sample");
        std::string tprof_key(key_base + "-tprof");
        int shm_num_rank = 0;

        init_prof_comm(std::move(comm), shm_num_rank);
        try {
            init_ctl_msg(sample_key);
            init_cpu_list(topo.num_domain(GEOPM_DOMAIN_CPU));
            init_cpu_affinity(shm_num_rank);
            init_tprof_table(tprof_key, topo);
            init_table(sample_key);
        }
        catch (const Exception &ex) {
            if (!m_rank) {
                std::cerr << "Warning: <geopm> Controller handshake failed, running without geopm." << std::endl;
                int err = ex.err_value();
                if (err != GEOPM_ERROR_RUNTIME) {
                    char tmp_msg[NAME_MAX];
                    geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
                    tmp_msg[NAME_MAX-1] = '\0';
                    std::cerr << tmp_msg << std::endl;
                }
            }
            m_is_enabled = false;
        }
#ifdef GEOPM_OVERHEAD
        m_overhead_time_startup = geopm_time_since(&overhead_entry);
#endif
    }


    ProfileImp::ProfileImp()
        : ProfileImp(environment().profile(), environment().shmkey(), environment().report(),
                     environment().timeout(), environment().do_region_barrier(),
                     comm_factory().make_plugin(environment().comm()), nullptr, platform_topo(), nullptr,
                     nullptr, geopm::make_unique<SampleSchedulerImp>(0.01), nullptr)
    {
    }

    void ProfileImp::init_prof_comm(std::unique_ptr<Comm> comm, int &shm_num_rank)
    {
        if (!m_shm_comm) {
            m_rank = comm->rank();
            m_shm_comm = comm->split("prof", Comm::M_COMM_SPLIT_TYPE_SHARED);
            comm->tear_down();
            comm.reset();
            m_shm_rank = m_shm_comm->rank();
            shm_num_rank = m_shm_comm->num_rank();
            m_shm_comm->barrier();
        }
    }

    void ProfileImp::init_ctl_msg(const std::string &sample_key)
    {
        if (!m_ctl_msg) {
            m_ctl_shmem = geopm::make_unique<SharedMemoryUserImp>(sample_key, m_timeout);
            m_shm_comm->barrier();
            if (!m_shm_rank) {
                m_ctl_shmem->unlink();
            }

            if (m_ctl_shmem->size() < sizeof(struct geopm_ctl_message_s)) {
                throw Exception("ProfileImp: ctl_shmem too small", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_ctl_msg = geopm::make_unique<ControlMessageImp>(*(struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), false, !m_shm_rank, m_timeout);
        }
    }

    void ProfileImp::init_cpu_list(int num_cpu)
    {
        if (!m_is_enabled) {
            return;
        }

        cpu_set_t *proc_cpuset = NULL;
        proc_cpuset = CPU_ALLOC(num_cpu);
        if (!proc_cpuset) {
            throw Exception("ProfileImp: unable to allocate process CPU mask",
                            ENOMEM, __FILE__, __LINE__);
        }
        geopm_sched_proc_cpuset(num_cpu, proc_cpuset);
        for (int i = 0; i < num_cpu; ++i) {
            if (CPU_ISSET(i, proc_cpuset)) {
                m_cpu_list.push_front(i);
            }
        }
        free(proc_cpuset);
    }

    void ProfileImp::init_cpu_affinity(int shm_num_rank)
    {
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_MAP_BEGIN
        m_ctl_msg->wait();  // M_STATUS_MAP_BEGIN

        for (int i = 0 ; i < shm_num_rank; ++i) {
            if (i == m_shm_rank) {
                if (i == 0) {
                    for (int i = 0; i < GEOPM_MAX_NUM_CPU; ++i) {
                        m_ctl_msg->cpu_rank(i, -1);
                    }
                    for (auto it = m_cpu_list.begin(); it != m_cpu_list.end(); ++it) {
                        m_ctl_msg->cpu_rank(*it, m_rank);
                    }
                }
                else {
                    for (auto it = m_cpu_list.begin(); it != m_cpu_list.end(); ++it) {
                        if (m_ctl_msg->cpu_rank(*it) != -1) {
                            m_ctl_msg->cpu_rank(*it, -2);
                        }
                        else {
                            m_ctl_msg->cpu_rank(*it, m_rank);
                        }
                    }
                }
            }
            m_shm_comm->barrier();
        }

        if (!m_shm_rank) {
            for (int i = 0; i < GEOPM_MAX_NUM_CPU; ++i) {
                if (m_ctl_msg->cpu_rank(i) == -2) {
                    throw Exception("ProfileImp: cpu_rank not initialized correctly.",
                                    GEOPM_ERROR_AFFINITY, __FILE__, __LINE__);
                }
            }
        }
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_MAP_END
        m_ctl_msg->wait();  // M_STATUS_MAP_END
    }

    void ProfileImp::init_tprof_table(const std::string &tprof_key, const PlatformTopo &topo)
    {
        if (!m_tprof_table) {
            m_tprof_shmem = geopm::make_unique<SharedMemoryUserImp>(tprof_key, m_timeout);
            m_shm_comm->barrier();
            if (!m_shm_rank) {
                m_tprof_shmem->unlink();
            }
            m_tprof_table = std::make_shared<ProfileThreadTableImp>(topo, m_tprof_shmem->size(), m_tprof_shmem->pointer());
        }
    }

    void ProfileImp::init_table(const std::string &sample_key)
    {
        if (!m_table) {
            std::string table_shm_key(sample_key);
            table_shm_key += "-" + std::to_string(m_rank);
            m_table_shmem = geopm::make_unique<SharedMemoryUserImp>(table_shm_key, m_timeout);
            m_table_shmem->unlink();
            m_table = geopm::make_unique<ProfileTableImp>(m_table_shmem->size(), m_table_shmem->pointer());
        }

        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_SAMPLE_BEGIN
        m_ctl_msg->wait();  // M_STATUS_SAMPLE_BEGIN
    }

    ProfileImp::~ProfileImp()
    {
        shutdown();
    }

    void ProfileImp::shutdown(void)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_SAMPLE_END
        m_ctl_msg->wait();  // M_SAMPLE_END

#ifdef GEOPM_OVERHEAD
        m_overhead_time_shutdown = geopm_time_since(&overhead_entry);
#endif

        print(m_report);
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_SHUTDOWN
        m_shm_comm->tear_down();
        m_shm_comm.reset();
        m_is_enabled = false;
    }

    uint64_t ProfileImp::region(const std::string region_name, long hint)
    {
        if (!m_is_enabled) {
            return 0;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        if (hint && ((hint & (hint - 1)) != 0)) {   /// power of 2 check
            throw Exception("ProfileImp:region() multiple region hints set and only 1 at a time is supported.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        uint64_t result = m_table->key(region_name);
        /// Record hint when registering a region.
        result = geopm_region_id_set_hint(hint, result);

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

        return result;
    }

    void ProfileImp::enter(uint64_t region_id)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        // if we are not currently in a region
        if (!m_curr_region_id && region_id) {
            if (!geopm_region_id_is_mpi(region_id) &&
                m_do_region_barrier) {
                m_shm_comm->barrier();
            }
            m_curr_region_id = region_id;
            m_num_enter = 0;
            m_progress = 0.0;
            sample();
        }
        else {
            m_tprof_table->enable(false);
            // Allow nesting of one MPI region within a non-mpi region
            if (m_curr_region_id &&
                geopm_region_id_hint(m_curr_region_id) != GEOPM_REGION_HINT_NETWORK &&
                geopm_region_id_is_mpi(region_id)) {
                m_parent_num_enter = m_num_enter;
                m_num_enter = 0;
                m_parent_region = m_curr_region_id;
                m_parent_progress = m_progress;
                m_curr_region_id = geopm_region_id_set_mpi(m_curr_region_id);
                m_progress = 0.0;
                sample();
            }
        }
        // keep track of number of entries to account for nesting
        if (m_curr_region_id == region_id ||
            (geopm_region_id_is_mpi(m_curr_region_id) &&
             geopm_region_id_is_mpi(region_id))) {
            ++m_num_enter;
        }

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::exit(uint64_t region_id)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        // keep track of number of exits to account for nesting
        if (m_curr_region_id == region_id ||
            (geopm_region_id_is_mpi(m_curr_region_id) &&
             geopm_region_id_is_mpi(region_id))) {
            --m_num_enter;
        }
        if (m_num_enter == 1) {
            m_tprof_table->enable(true);
        }

        // if we are leaving the outer most nesting of our current region
        if (!m_num_enter) {
            if (geopm_region_id_is_mpi(region_id)) {
                m_curr_region_id = geopm_region_id_set_mpi(m_parent_region);
            }
            m_progress = 1.0;
            sample();
            m_curr_region_id = 0;
            m_scheduler->clear();
            if (geopm_region_id_is_mpi(region_id)) {
                m_curr_region_id = m_parent_region;
                m_progress = m_parent_progress;
                m_num_enter = m_parent_num_enter;
                m_parent_region = 0;
                m_parent_progress = 0.0;
                m_parent_num_enter = 0;
            }

            if (!geopm_region_id_is_mpi(region_id) &&
                m_do_region_barrier) {
                m_shm_comm->barrier();
            }

        }

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::progress(uint64_t region_id, double fraction)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        if (m_num_enter == 1 && m_curr_region_id == region_id &&
            fraction > 0.0 && fraction < 1.0 &&
            m_scheduler->do_sample()) {
            m_progress = fraction;
            sample();
            m_scheduler->record_exit();
        }

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::epoch(void)
    {
        if (!m_is_enabled ||
            geopm_region_id_hint_is_equal(m_curr_region_id,
                                          GEOPM_REGION_HINT_IGNORE)) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        struct geopm_prof_message_s sample;
        sample.rank = m_rank;
        sample.region_id = GEOPM_REGION_ID_EPOCH;
        (void) geopm_time(&(sample.timestamp));
        sample.progress = 0.0;
        m_table->insert(sample);

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::sample(void)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        struct geopm_prof_message_s sample;
        sample.rank = m_rank;
        sample.region_id = m_curr_region_id;
        (void) geopm_time(&(sample.timestamp));
        sample.progress = m_progress;
        m_table->insert(sample);

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::print(const std::string file_name)
    {
        if (!m_is_enabled || !m_table_shmem) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        int is_done = 0;
        int is_all_done = 0;

        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_NAME_BEGIN
        m_ctl_msg->wait();  // M_STATUS_NAME_BEGIN

        size_t buffer_offset = 0;
        size_t buffer_remain = m_table_shmem->size();
        char *buffer_ptr = (char *)(m_table_shmem->pointer());

        if (m_table_shmem->size() < file_name.length() + 1 + m_prof_name.length() + 1) {
            throw Exception("ProfileImp:print() profile file name and profile name are too long to fit in a table buffer", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        strncpy(buffer_ptr, file_name.c_str(), buffer_remain - 1);
        buffer_ptr += file_name.length() + 1;
        buffer_offset += file_name.length() + 1;
        buffer_remain -= file_name.length() + 1;
        strncpy(buffer_ptr, m_prof_name.c_str(), buffer_remain - 1);
        buffer_offset += m_prof_name.length() + 1;
        while (!is_all_done) {
            m_shm_comm->barrier();
            m_ctl_msg->loop_begin();  // M_STATUS_NAME_LOOP_BEGIN

            is_done = m_table->name_fill(buffer_offset);
            is_all_done = m_shm_comm->test(is_done);

            m_ctl_msg->step();  // M_STATUS_NAME_LOOP_END
            m_ctl_msg->wait();  // M_STATUS_NAME_LOOP_END
            buffer_offset = 0;
        }
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_NAME_END
        m_ctl_msg->wait();  // M_STATUS_NAME_END

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
        double overhead_buffer[3] = {m_overhead_time_startup,
                                     m_overhead_time,
                                     m_overhead_time_shutdown};
        double max_overhead[3] = {};
        m_reduce_comm->reduce_max(overhead_buffer, max_overhead, 3, 0);

        if (!m_rank) {
            std::cout << "GEOPM startup (seconds):  " << max_overhead[0] << std::endl;
            std::cout << "GEOPM runtime (seconds):  " << max_overhead[1] << std::endl;
            std::cout << "GEOPM shutdown (seconds): " << max_overhead[2] << std::endl;
        }
#endif

    }

    std::shared_ptr<ProfileThreadTable> ProfileImp::tprof_table(void)
    {
        return m_tprof_table;
    }
}
