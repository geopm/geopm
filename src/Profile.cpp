/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <iostream>
#include <sstream>

#include <float.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "geopm.h"
#include "geopm_ctl.h"
#include "geopm_message.h"
#include "geopm_signal_handler.h"
#include "geopm_sched.h"
#include "geopm_env.h"
#include "Profile.hpp"
#include "ProfileSampler.hpp"
#include "ProfileTable.hpp"
#include "ProfileThread.hpp"
#include "SampleScheduler.hpp"
#include "ControlMessage.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "Comm.hpp"

#include "config.h"


static geopm::Profile &geopm_default_prof(void)
{
    const geopm::IComm *tmp_comm = geopm::geopm_get_comm("MPIComm");
    static geopm::Profile default_prof(geopm_env_profile(), tmp_comm);
    return default_prof;
}

extern "C"
{
    // defined in geopm_pmpi.c and used only here
    void geopm_pmpi_prof_enable(int do_profile);

    int geopm_prof_init(void)
    {
        int err = 0;
        try {
            geopm_default_prof();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_region(const char *region_name, uint64_t hint, uint64_t *region_id)
    {
        int err = 0;
        try {
            *region_id = geopm_default_prof().region(std::string(region_name), hint);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_enter(uint64_t region_id)
    {
        int err = 0;
        try {
            geopm_default_prof().enter(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_exit(uint64_t region_id)
    {
        int err = 0;
        try {
            geopm_default_prof().exit(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_progress(uint64_t region_id, double fraction)
    {
        int err = 0;
        try {
            geopm_default_prof().progress(region_id, fraction);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_epoch(void)
    {
        int err = 0;
        try {
            geopm_default_prof().epoch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_shutdown(void)
    {
        int err = 0;
        try {
            geopm_default_prof().shutdown();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_init(uint32_t num_work_unit)
    {
        int err = 0;
        try {
            geopm_default_prof().tprof_table()->init(num_work_unit);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_tprof_init_loop(int num_thread, int thread_idx, size_t num_iter, size_t chunk_size)
    {
        int err = 0;
        try {
            geopm::IProfileThreadTable *table_ptr = geopm_default_prof().tprof_table();
            if (chunk_size) {
                table_ptr->init(num_thread, thread_idx, num_iter, chunk_size);
            }
            else {
                table_ptr->init(num_thread, thread_idx, num_iter);
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_post(void)
    {
        int err = 0;
        try {
            geopm_default_prof().tprof_table()->post();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

}

namespace geopm
{
    Profile::Profile(const std::string prof_name, const IComm *comm)
        : m_is_enabled(true)
        , m_prof_name(prof_name)
        , m_curr_region_id(0)
        , m_num_enter(0)
        , m_num_progress(0)
        , m_progress(0.0)
        , m_table_buffer(NULL)
        , m_ctl_shmem(NULL)
        , m_ctl_msg(NULL)
        , m_table_shmem(NULL)
        , m_table(NULL)
        , m_tprof_shmem(NULL)
        , m_tprof_table(NULL)
        , M_OVERHEAD_FRAC(0.01)
        , m_scheduler(NULL)
        , m_shm_comm(NULL)
        , m_rank(0)
        , m_shm_rank(0)
        , m_is_first_sync(true)
        , m_parent_region(0)
        , m_parent_progress(0.0)
        , m_parent_num_enter(0)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
    {
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif
        int shm_num_rank = 0;

        m_scheduler = new SampleScheduler(M_OVERHEAD_FRAC);
        m_rank = comm->rank();
        m_shm_comm = comm->split("prof", IComm::M_COMM_SPLIT_TYPE_SHARED);
        m_shm_rank = m_shm_comm->rank();
        shm_num_rank = m_shm_comm->num_rank();
        m_shm_comm->barrier();

        std::string key(geopm_env_shmkey());
        key += "-sample";
        try {
            m_ctl_shmem = new SharedMemoryUser(key, geopm_env_profile_timeout()); // 5 second timeout
        }
        catch (Exception ex) {
            if (ex.err_value() != ENOENT) {
                throw ex;
            }
            if (!m_rank) {
                std::cerr << "Warning: <geopm> Controller not found, running without geopm." << std::endl;
            }
            m_is_enabled = false;
            return;
        }
        m_shm_comm->barrier();
        if (!m_shm_rank) {
            m_ctl_shmem->unlink();
        }

        try {
            m_ctl_msg = new ControlMessage((struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), false, !m_shm_rank);

            init_cpu_list();

            m_shm_comm->barrier();
            m_ctl_msg->step();
            m_ctl_msg->wait();

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
                        if (geopm_env_do_ignore_affinity()) {
                            for (int j = 0; j < shm_num_rank; ++j) {
                                m_ctl_msg->cpu_rank(j, j);
                            }
                            break;
                        }
                        else {
                            throw Exception("Profile: set GEOPM_ERROR_AFFINITY_IGNORE to ignore error", GEOPM_ERROR_AFFINITY, __FILE__, __LINE__);
                        }
                    }
                }
            }
            m_shm_comm->barrier();
            m_ctl_msg->step();
            m_ctl_msg->wait();

            std::ostringstream table_shm_key;
            table_shm_key << key <<  "-"  << m_rank;
            m_table_shmem = new SharedMemoryUser(table_shm_key.str(), 3.0);
            m_shm_comm->barrier();
            if (!m_shm_rank) {
                m_table_shmem->unlink();
            }
            m_shm_comm->barrier();

            m_table_buffer = m_table_shmem->pointer();
            m_table = new ProfileTable(m_table_shmem->size(), m_table_buffer);

            std::string tprof_key(geopm_env_shmkey());
            tprof_key += "-tprof";
            m_tprof_shmem = new SharedMemoryUser(tprof_key, 3.0);
            m_shm_comm->barrier();
            if (!m_shm_rank) {
                m_tprof_shmem->unlink();
            }
            m_tprof_table = new ProfileThreadTable(m_tprof_shmem->size(), m_tprof_shmem->pointer());

            m_shm_comm->barrier();
            m_ctl_msg->step();
            m_ctl_msg->wait();
            geopm_pmpi_prof_enable(1);
#ifdef GEOPM_OVERHEAD
            struct geopm_time_s overhead_exit;
            geopm_time(&overhead_exit);
            m_overhead_time_startup = geopm_time_diff(&overhead_entry, &overhead_exit);
#endif
        }
        catch (Exception ex) {
            if (ex.err_value() != GEOPM_ERROR_RUNTIME) {
                throw ex;
            }
            if (!m_rank) {
                std::cerr << "Warning: <geopm> Controller handshake failed, running without geopm." << std::endl;
            }
            m_is_enabled = false;
        }
    }

    Profile::~Profile()
    {
        shutdown();
        delete m_tprof_table;
        delete m_tprof_shmem;
        delete m_table;
        delete m_table_shmem;
        delete m_ctl_shmem;
        delete m_scheduler;
    }

    void Profile::shutdown(void)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        geopm_pmpi_prof_enable(0);
        m_shm_comm->barrier();
        m_ctl_msg->step();
        m_ctl_msg->wait();

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time_shutdown = geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

        if (geopm_env_report_verbosity()) {
            print(geopm_env_report(), geopm_env_report_verbosity());
        }
        m_shm_comm->barrier();
        m_ctl_msg->step();
        m_is_enabled = false;
    }

    uint64_t Profile::region(const std::string region_name, long hint)
    {
        if (!m_is_enabled) {
            return 0;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        uint64_t result = m_table->key(region_name);
        /// Record hint when registering a region.
        result = geopm_region_id_set_hint(hint, result);

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

        return result;
    }

    void Profile::enter(uint64_t region_id)
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
                geopm_env_do_region_barrier()) {
                m_shm_comm->barrier();
            }
            m_curr_region_id = region_id;
            m_num_enter = 0;
            m_progress = 0.0;
            sample();
        }
        else
        {
            m_tprof_table->enable(false);
            // Allow nesting of one MPI region within a non-mpi region
            if (m_curr_region_id &&
                !geopm_region_id_is_mpi(m_curr_region_id) &&
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
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

    }

    void Profile::exit(uint64_t region_id)
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
            if (!geopm_region_id_is_mpi(region_id) &&
                geopm_env_do_region_barrier()) {
                m_shm_comm->barrier();
            }
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
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

    }

    void Profile::progress(uint64_t region_id, double fraction)
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
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

    }

    void Profile::epoch(void)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        struct geopm_prof_message_s sample;
        m_shm_comm->barrier();
        if (!m_shm_rank) {
            sample.rank = m_rank;
            sample.region_id = GEOPM_REGION_ID_EPOCH;
            (void) geopm_time(&(sample.timestamp));
            sample.progress = 0.0;
            m_table->insert(sample.region_id, sample);
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

    }

    void Profile::sample(void)
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
        m_table->insert(m_curr_region_id, sample);

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
#endif

    }

    void Profile::print(const std::string file_name, int depth)
    {
        if (!m_is_enabled) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

        int is_done = 0;
        int is_all_done = 0;

        m_shm_comm->barrier();
        m_ctl_msg->step();
        m_ctl_msg->wait();

        size_t buffer_offset = 0;
        size_t buffer_remain = m_table_shmem->size();
        char *buffer_ptr = (char *)(m_table_shmem->pointer());

        if (m_table_shmem->size() < file_name.length() + 1 + m_prof_name.length() + 1) {
            throw Exception("Profile:print() profile file name and profile name are too long to fit in a table buffer", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        strncpy(buffer_ptr, file_name.c_str(), buffer_remain - 1);
        buffer_ptr += file_name.length() + 1;
        buffer_offset += file_name.length() + 1;
        buffer_remain -= file_name.length() + 1;
        strncpy(buffer_ptr, m_prof_name.c_str(), buffer_remain - 1);
        buffer_offset += m_prof_name.length() + 1;
        while (!is_all_done) {
            m_shm_comm->barrier();
            m_ctl_msg->loop_begin();

            is_done = m_table->name_fill(buffer_offset);
            is_all_done = m_shm_comm->test(is_done);

            m_ctl_msg->step();
            m_ctl_msg->wait();
            buffer_offset = 0;
        }
        m_shm_comm->barrier();
        m_ctl_msg->step();
        m_ctl_msg->wait();

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time += geopm_time_diff(&overhead_entry, &overhead_exit);
        double overhead_buffer[3] = {m_overhead_time_startup,
                                     m_overhead_time,
                                     m_overhead_time_shutdown};
        double max_overhead[3] = {};
        const IComm *tmp_comm = geopm::geopm_get_comm("MPIComm");
        tmp_comm->reduce_max(overhead_buffer, max_overhead, 3, 0);
        if (!m_rank) {
            std::cout << "GEOPM startup (seconds):  " << max_overhead[0] << std::endl;
            std::cout << "GEOPM runtime (seconds):  " << max_overhead[1] << std::endl;
            std::cout << "GEOPM shutdown (seconds): " << max_overhead[2] << std::endl;
        }
#endif

    }

    void Profile::init_cpu_list(void)
    {
        if (!m_is_enabled) {
            return;
        }

        cpu_set_t *proc_cpuset = NULL;
        int num_cpu = geopm_sched_num_cpu();
        proc_cpuset = CPU_ALLOC(num_cpu);
        if (!proc_cpuset) {
            throw Exception("Profile: unable to allocate process CPU mask",
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

    IProfileThreadTable *Profile::tprof_table(void)
    {
        return m_tprof_table;
    }
}
