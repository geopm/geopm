/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <algorithm>
#include <iostream>
#include <sstream>

#include <float.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "geopm.h"
#include "geopm_message.h"
#include "geopm_time.h"
#include "geopm_signal_handler.h"
#include "geopm_sched.h"
#include "geopm_env.h"
#include "Profile.hpp"
#include "ProfileTable.hpp"
#include "ProfileThread.hpp"
#include "SampleScheduler.hpp"
#include "ControlMessage.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "Comm.hpp"

#include "config.h"

static bool geopm_prof_compare(const std::pair<uint64_t, struct geopm_prof_message_s> &aa, const std::pair<uint64_t, struct geopm_prof_message_s> &bb)
{
    return geopm_time_comp(&(aa.second.timestamp), &(bb.second.timestamp));
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
        , m_ctl_msg(nullptr)
        , m_table_shmem(NULL)
        , m_table(nullptr)
        , m_tprof_shmem(NULL)
        , m_tprof_table(NULL)
        , M_OVERHEAD_FRAC(0.01)
        , m_scheduler(nullptr)
        , m_shm_comm(nullptr)
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

        m_scheduler = std::unique_ptr<SampleScheduler>(new SampleScheduler(M_OVERHEAD_FRAC));
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

        m_ctl_msg = std::unique_ptr<ControlMessage>(new ControlMessage((struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), false, !m_shm_rank));

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

        std::string tprof_key(geopm_env_shmkey());
        tprof_key += "-tprof";
        m_tprof_shmem = new SharedMemoryUser(tprof_key, 3.0);
        m_shm_comm->barrier();
        if (!m_shm_rank) {
            m_tprof_shmem->unlink();
        }
        m_tprof_table = new ProfileThreadTable(m_tprof_shmem->size(), m_tprof_shmem->pointer());

        std::ostringstream table_shm_key;
        table_shm_key << key <<  "-"  << m_rank;
        m_table_shmem = new SharedMemoryUser(table_shm_key.str(), 3.0);
        m_table_shmem->unlink();

        m_table_buffer = m_table_shmem->pointer();
        m_table = std::unique_ptr<ProfileTable>(new ProfileTable(m_table_shmem->size(), m_table_buffer));

        m_shm_comm->barrier();
        m_ctl_msg->step();
        m_ctl_msg->wait();
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_exit;
        geopm_time(&overhead_exit);
        m_overhead_time_startup = geopm_time_diff(&overhead_entry, &overhead_exit);
#endif
    }

    Profile::Profile(const std::string prof_name, IProfileThreadTable *prof_table, ISharedMemoryUser *tprof_shmem, std::unique_ptr<IProfileTable> table, ISharedMemoryUser *table_shmem,
            std::unique_ptr<ISampleScheduler> scheduler, std::unique_ptr<IControlMessage> ctl_msg, ISharedMemoryUser *ctl_shmem, std::shared_ptr<IComm> comm)
        : m_is_enabled(true)
        , m_prof_name(prof_name)
        , m_curr_region_id(0)
        , m_num_enter(0)
        , m_num_progress(0)
        , m_progress(0.0)
        , m_table_buffer(malloc(1))
        , m_ctl_shmem(ctl_shmem)
        , m_ctl_msg(std::move(ctl_msg))
        , m_table_shmem(table_shmem)
        , m_table(std::move(table))
        , m_tprof_shmem(tprof_shmem)
        , m_tprof_table(prof_table)
        , M_OVERHEAD_FRAC(0.01)
        , m_scheduler(std::move(scheduler))
        , m_shm_comm(comm->split("prof", IComm::M_COMM_SPLIT_TYPE_SHARED))
        , m_rank(comm->rank())
        , m_shm_rank(m_shm_comm->rank())
        , m_is_first_sync(true)
        , m_parent_region(0)
        , m_parent_progress(0.0)
        , m_parent_num_enter(0)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
    {
        init_cpu_list();
    }

    Profile::~Profile()
    {
        shutdown();
        delete m_tprof_table;
        delete m_tprof_shmem;
        delete m_table_shmem;
        delete m_ctl_shmem;
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
        m_shm_comm.reset();
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
        else {
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
        CommFactory::comm_factory().get_comm(geopm_env_comm())->
            reduce_max(overhead_buffer, max_overhead, 3, 0);
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


    const struct geopm_prof_message_s GEOPM_INVALID_PROF_MSG = {-1, 0, {{0, 0}}, -1.0};

    ProfileSampler::ProfileSampler(size_t table_size)
        : m_table_size(table_size)
        , m_do_report(false)
        , m_tprof_shmem(NULL)
        , m_tprof_table(NULL)
    {
        std::string sample_key(geopm_env_shmkey());
        sample_key += "-sample";
        std::string sample_key_path("/dev/shm/" + sample_key);
        // Remove shared memory file if one already exists.
        (void)unlink(sample_key_path.c_str());
        m_ctl_shmem = new SharedMemory(sample_key, table_size);
        m_ctl_msg = new ControlMessage((struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), true, true);

        std::string tprof_key(geopm_env_shmkey());
        tprof_key += "-tprof";
        std::string tprof_key_path("/dev/shm/" + tprof_key);
        // Remove shared memory file if one already exists.
        (void)unlink(tprof_key_path.c_str());
        size_t tprof_size = 64 * geopm_sched_num_cpu();
        m_tprof_shmem = new SharedMemory(tprof_key, tprof_size);
        m_tprof_table = new ProfileThreadTable(tprof_size, m_tprof_shmem->pointer());
        errno = 0; // Ignore errors from the unlink calls.
    }

    ProfileSampler::~ProfileSampler(void)
    {
        delete m_tprof_table;
        delete m_tprof_shmem;
        for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
            delete (*it);
        }
        delete m_ctl_shmem;
    }

    void ProfileSampler::initialize(int &rank_per_node)
    {
        std::ostringstream shm_key;

        m_ctl_msg->wait();
        m_ctl_msg->step();
        m_ctl_msg->wait();

        std::set<int> rank_set;
        for (int i = 0; i < GEOPM_MAX_NUM_CPU; i++) {
            if (m_ctl_msg->cpu_rank(i) >= 0) {
                (void) rank_set.insert(m_ctl_msg->cpu_rank(i));
            }
        }

        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            shm_key.str("");
            shm_key << m_ctl_shmem->key() <<  "-"  << *it;
            m_rank_sampler.push_front(new ProfileRankSampler(shm_key.str(), m_table_size));
        }
        rank_per_node = rank_set.size();
        m_ctl_msg->step();
        m_ctl_msg->wait();
        m_ctl_msg->step();
    }

    void ProfileSampler::cpu_rank(std::vector<int> &cpu_rank)
    {
        uint32_t num_cpu = geopm_sched_num_cpu();
        cpu_rank.resize(num_cpu);
        if (num_cpu > GEOPM_MAX_NUM_CPU) {
            throw Exception("ProfileSampler::cpu_rank: Number of online CPUs is greater than GEOPM_MAX_NUM_CPU", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (unsigned cpu = 0; cpu < num_cpu; ++cpu) {
            cpu_rank[cpu] = m_ctl_msg->cpu_rank(cpu);
        }
    }

    size_t ProfileSampler::capacity(void)
    {
        size_t result = 0;
        for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
            result += (*it)->capacity();
        }
        return result;
    }

    void ProfileSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, std::shared_ptr<IComm> comm)
    {
        length = 0;
        if (m_ctl_msg->is_sample_begin() ||
            m_ctl_msg->is_sample_end()) {
            auto content_it = content.begin();
            for (auto rank_sampler_it = m_rank_sampler.begin();
                 rank_sampler_it != m_rank_sampler.end();
                 ++rank_sampler_it) {
                size_t rank_length = 0;
                (*rank_sampler_it)->sample(content_it, rank_length);
                content_it += rank_length;
                length += rank_length;
            }
            if (m_ctl_msg->is_sample_end()) {
                comm->barrier();
                m_ctl_msg->step();
                while (!m_ctl_msg->is_name_begin() &&
                       !m_ctl_msg->is_shutdown()) {
                    geopm_signal_handler_check();
                }
                if (m_ctl_msg->is_name_begin()) {
                    region_names();
                }
            }
        }
        else if (!m_ctl_msg->is_shutdown()) {
            throw Exception("ProfileSampler: invalid application status, expected shutdown status", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    bool ProfileSampler::do_shutdown(void)
    {
        return m_ctl_msg->is_shutdown();
    }

    bool ProfileSampler::do_report(void)
    {
        return m_do_report;
    }

    void ProfileSampler::region_names(void)
    {
        m_ctl_msg->step();

        bool is_all_done = false;
        while (!is_all_done) {
            m_ctl_msg->loop_begin();
            m_ctl_msg->wait();
            is_all_done = true;
            for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
                if (!(*it)->name_fill(m_name_set)) {
                    is_all_done = false;
                }
            }
            m_ctl_msg->step();
            if (!is_all_done && m_ctl_msg->is_shutdown()) {
                throw Exception("ProfileSampler::region_names(): Application shutdown while report was being generated", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_rank_sampler.front()->report_name(m_report_name);
        m_rank_sampler.front()->profile_name(m_profile_name);
        m_do_report = true;

        m_ctl_msg->wait();
        m_ctl_msg->step();
        m_ctl_msg->wait();
    }

    void ProfileSampler::name_set(std::set<std::string> &region_name)
    {
        region_name = m_name_set;
    }

    void ProfileSampler::report_name(std::string &report_str)
    {
        report_str = m_report_name;
    }

    void ProfileSampler::profile_name(std::string &prof_str)
    {
        prof_str = m_profile_name;
    }

    IProfileThreadTable *ProfileSampler::tprof_table(void)
    {
        return m_tprof_table;
    }

    ProfileRankSampler::ProfileRankSampler(const std::string shm_key, size_t table_size)
        : m_table_shmem(NULL)
        , m_table(NULL)
        , m_region_entry(GEOPM_INVALID_PROF_MSG)
        , m_is_name_finished(false)
    {
        std::string key_path("/dev/shm/" + shm_key);
        (void)unlink(key_path.c_str());
        errno = 0; // Ignore errors from the unlink call.
        m_table_shmem = new SharedMemory(shm_key, table_size);
        m_table = new ProfileTable(m_table_shmem->size(), m_table_shmem->pointer());
    }

    ProfileRankSampler::~ProfileRankSampler()
    {
        delete m_table;
        delete m_table_shmem;
    }

    size_t ProfileRankSampler::capacity(void)
    {
        return m_table->capacity();
    }

    void ProfileRankSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length)
    {
        m_table->dump(content_begin, length);
        std::stable_sort(content_begin, content_begin + length, geopm_prof_compare);
    }

    bool ProfileRankSampler::name_fill(std::set<std::string> &name_set)
    {
        size_t header_offset = 0;

        if (!m_is_name_finished) {
            if (name_set.empty()) {
                m_report_name = (char *)m_table_shmem->pointer();
                header_offset += m_report_name.length() + 1;
                m_prof_name = (char *)m_table_shmem->pointer() + header_offset;
                header_offset += m_prof_name.length() + 1;
            }
            m_is_name_finished = m_table->name_set(header_offset, name_set);
        }

        return m_is_name_finished;
    }

    void ProfileRankSampler::report_name(std::string &report_str)
    {
        report_str = m_report_name;
    }

    void ProfileRankSampler::profile_name(std::string &prof_str)
    {
        prof_str = m_prof_name;
    }
}
