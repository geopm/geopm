/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <float.h>
#include <unistd.h>
#include <hwloc.h>
#include <iostream>

#include "geopm.h"
#include "geopm_sched.h"
#include "geopm_message.h"
#include "geopm_time.h"
#include "Profile.hpp"
#include "ProfileThread.hpp"
#include "Exception.hpp"
#include "geopm_env.h"
#include "LockingHashTable.hpp"
#include "config.h"

static struct geopm_prof_c *g_geopm_prof_default = NULL;

static bool geopm_prof_compare(const std::pair<uint64_t, struct geopm_prof_message_s> &aa, const std::pair<uint64_t, struct geopm_prof_message_s> &bb)
{
    return geopm_time_comp(&(aa.second.timestamp), &(bb.second.timestamp));
}

extern "C"
{
    // defined in geopm_pmpi.c and used only here
    void geopm_pmpi_prof_enable(int do_profile);
#ifndef GEOPM_PMPI
    void geopm_pmpi_prof_enable(int do_profile) {}
#endif


    int geopm_prof_create(const char *name, const char *shm_key, MPI_Comm comm, struct geopm_prof_c **prof)
    {
        int err = 0;
        try {
            *prof = (struct geopm_prof_c *)(new geopm::Profile(std::string(name), std::string(shm_key ? shm_key : ""), comm));
            /*! @todo Code below is not thread safe, do we need a lock? */
            if (g_geopm_prof_default == NULL) {
                g_geopm_prof_default = *prof;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_create_f(const char *name, const char *shm_key, int comm, struct geopm_prof_c **prof)
    {
        return geopm_prof_create(name, shm_key, MPI_Comm_f2c(comm), prof);
    }


    int geopm_prof_destroy(struct geopm_prof_c *prof)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            delete prof_obj;
            if (prof == g_geopm_prof_default) {
                g_geopm_prof_default = NULL;
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_default(struct geopm_prof_c *prof)
    {
        int err = 0;
        if (prof) {
            /*! @todo Code below is not thread safe, do we need a lock? */
            g_geopm_prof_default = prof;
        }
        else {
            err = g_geopm_prof_default ? 0 : GEOPM_ERROR_LOGIC;
        }
        return err;
    }

    int geopm_prof_region(struct geopm_prof_c *prof, const char *region_name, long policy_hint, uint64_t *region_id)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            *region_id = prof_obj->region(std::string(region_name), policy_hint);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_enter(struct geopm_prof_c *prof, uint64_t region_id)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->enter(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_exit(struct geopm_prof_c *prof, uint64_t region_id)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->exit(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_progress(struct geopm_prof_c *prof, uint64_t region_id, double fraction)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->progress(region_id, fraction);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_outer_sync(struct geopm_prof_c *prof)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->outer_sync();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_disable(struct geopm_prof_c *prof, const char *feature_name)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->disable(std::string(feature_name));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_print(struct geopm_prof_c *prof, const char *file_name, int depth)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->print(std::string(file_name), depth);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_tprof_create(int num_thread, size_t num_iter, size_t chunk_size, struct geopm_tprof_c **tprof)
    {
        int err = 0;
        try {
            *tprof = (struct geopm_tprof_c *)(new geopm::ProfileThread(num_thread, num_iter, chunk_size));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_destroy(struct geopm_tprof_c *tprof)
    {
        int err = 0;
        try {
            geopm::ProfileThread *tprof_obj = (geopm::ProfileThread *)(tprof);
            if (tprof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            delete tprof_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_increment(struct geopm_tprof_c *tprof, struct geopm_prof_c *prof, uint64_t region_id, int thread_idx)
    {
        int err = 0;
        try {
            geopm::ProfileThread *tprof_obj = (geopm::ProfileThread *)(tprof);
            geopm::Profile *prof_obj = (geopm::Profile *)(prof ? prof : g_geopm_prof_default);
            if (tprof_obj == NULL || prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            tprof_obj->increment(*prof_obj, region_id, thread_idx);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
}

namespace geopm
{

    static const std::string g_default_shmem_key("/geopm_default");

    Profile::Profile(const std::string prof_name, const std::string shm_key, MPI_Comm comm)
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
        , m_scheduler(0.01)
        , m_shm_comm(MPI_COMM_NULL)
        , m_rank(0)
        , m_shm_rank(0)
        , m_is_first_sync(true)
        , m_parent_region(0)
        , m_parent_progress(0.0)
        , m_parent_num_enter(0)
    {
        int shm_num_rank = 0;

        MPI_Comm_rank(comm, &m_rank);
        geopm_comm_split_shared(comm, &m_shm_comm);
        MPI_Comm_rank(m_shm_comm, &m_shm_rank);
        MPI_Comm_size(m_shm_comm, &shm_num_rank);

        std::string key(shm_key);
        if (!key.size()) {
            key = geopm_env_shmkey();
        }
        if (!key.size()) {
            key = g_default_shmem_key;
        }
        key = key + "-sample";
        try {
            m_ctl_shmem = new SharedMemoryUser(key, 5); // 5 second timeout
        }
        catch (Exception ex) {
            if (ex.err_value() != ENOENT) {
                throw ex;
            }
#ifdef GEOPM_DEBUG
            if (!m_rank) {
                std::cerr << "Warning: <geopm> Controller not found, running without geopm." << std::endl;
            }
#endif
            m_is_enabled = false;
            return;
        }

        m_ctl_msg = (struct geopm_ctl_message_s *)m_ctl_shmem->pointer();

        init_cpu_list();

        for (int i = 0 ; i < shm_num_rank; ++i) {
            if (i == m_shm_rank) {
                if (i == 0) {
                    for (int i = 0; i < GEOPM_MAX_NUM_CPU; ++i) {
                        m_ctl_msg->cpu_rank[i] = -1;
                    }
                    for (auto it = m_cpu_list.begin(); it != m_cpu_list.end(); ++it) {
                        m_ctl_msg->cpu_rank[*it] = m_rank;
                    }
                }
                else {
                    for (auto it = m_cpu_list.begin(); it != m_cpu_list.end(); ++it) {
                        if (m_ctl_msg->cpu_rank[*it] != -1) {
                            m_ctl_msg->cpu_rank[*it] = -2;
                        }
                        else {
                            m_ctl_msg->cpu_rank[*it] = m_rank;
                        }
                    }
                }
            }
            MPI_Barrier(m_shm_comm);
        }

        if (!m_shm_rank) {
            for (int i = 0; i < GEOPM_MAX_NUM_CPU; ++i) {
                if (m_ctl_msg->cpu_rank[i] == -2) {
                    if (geopm_env_do_ignore_affinity()) {
                        for (int j = 0; j < shm_num_rank; ++j) {
                            m_ctl_msg->cpu_rank[j] = j;
                        }
                        break;
                    }
                    else {
                        throw Exception("Profile: set GEOPM_ERROR_AFFINITY_IGNORE to ignore error", GEOPM_ERROR_AFFINITY, __FILE__, __LINE__);
                    }
                }
            }
            m_ctl_msg->app_status = GEOPM_STATUS_INITIALIZED;
        }

        while (m_ctl_msg->ctl_status != GEOPM_STATUS_INITIALIZED) {}
        std::string table_shm_key(key + "-" + std::to_string(m_rank));
        m_table_shmem = new SharedMemoryUser(table_shm_key, 3.0);
        m_table_buffer = m_table_shmem->pointer();
        m_table = new ProfileTable(m_table_shmem->size(), m_table_buffer);
        MPI_Barrier(m_shm_comm);
        if (!m_shm_rank) {
            m_ctl_msg->app_status = GEOPM_STATUS_ACTIVE;
        }

        while (m_ctl_msg->ctl_status != GEOPM_STATUS_ACTIVE) {}
        geopm_pmpi_prof_enable(1);
    }

    Profile::~Profile()
    {
        if (m_is_enabled && !m_shm_rank) {
            m_ctl_msg->app_status = GEOPM_STATUS_SHUTDOWN;
        }
        delete m_table;
        delete m_table_shmem;
        delete m_ctl_shmem;
    }

    uint64_t Profile::region(const std::string region_name, long policy_hint)
    {
        if (!m_is_enabled) {
            return 0;
        }
        return m_table->key(region_name);
        /// @todo Record policy hint when registering a region.
    }

    void Profile::enter(uint64_t region_id)
    {
        if (!m_is_enabled) {
           return;
        }
        // if we are not currently in a region
        if (!m_curr_region_id && region_id) {
            if (geopm_env_do_region_barrier()) {
                PMPI_Barrier(m_shm_comm);
            }
            m_curr_region_id = region_id;
            m_num_enter = 0;
            m_progress = 0.0;
            sample(region_id);
        }
        // Allow nesting of one MPI region within a non-mpi region
        else if (m_curr_region_id &&
                 m_curr_region_id != GEOPM_REGION_ID_MPI &&
                 region_id == GEOPM_REGION_ID_MPI) {
            m_parent_num_enter = m_num_enter;
            m_num_enter = 0;
            m_parent_region = m_curr_region_id;
            m_parent_progress = m_progress;
            m_curr_region_id = region_id;
            m_progress = 0.0;
            sample(region_id);
        }
        // keep track of number of entries to account for nesting
        if (m_curr_region_id == region_id) {
            ++m_num_enter;
        }
    }

    void Profile::exit(uint64_t region_id)
    {
        if (!m_is_enabled) {
           return;
        }

        // keep track of number of exits to account for nesting
        if (m_curr_region_id == region_id) {
            --m_num_enter;
        }
        // if we are leaving the outer most nesting of our current region
        if (!m_num_enter) {
            if (region_id != GEOPM_REGION_ID_MPI && geopm_env_do_region_barrier()) {
                PMPI_Barrier(m_shm_comm);
            }
            m_progress = 1.0;
            sample(region_id);
            m_curr_region_id = 0;
            m_scheduler.clear();
            if (region_id == GEOPM_REGION_ID_MPI) {
                m_curr_region_id = m_parent_region;
                m_progress = m_parent_progress;
                m_num_enter = m_parent_num_enter;
                m_parent_region = 0;
                m_parent_progress = 0.0;
                m_parent_num_enter = 0;
            }
        }
    }

    void Profile::progress(uint64_t region_id, double fraction)
    {
        if (!m_is_enabled) {
           return;
        }

        if (m_num_enter == 1 && m_curr_region_id == region_id &&
            fraction > 0.0 && fraction < 1.0 &&
            m_scheduler.do_sample()) {
            m_progress = fraction;
            sample(region_id);
            m_scheduler.record_exit();
        }
    }

    void Profile::outer_sync(void)
    {
        if (!m_is_enabled) {
           return;
        }

        struct geopm_prof_message_s sample;
        PMPI_Barrier(m_shm_comm);
        if (!m_shm_rank) {
            sample.rank = m_rank;
            sample.region_id = GEOPM_REGION_ID_OUTER;
            (void) geopm_time(&(sample.timestamp));
            sample.progress = 0.0;
            m_table->insert(sample.region_id, sample);
        }
    }

    void Profile::sample(uint64_t region_id)
    {
        if (!m_is_enabled) {
           return;
        }

        if (region_id == m_curr_region_id) {
            struct geopm_prof_message_s sample;
            sample.rank = m_rank;
            sample.region_id = region_id;
            (void) geopm_time(&(sample.timestamp));
            sample.progress = m_progress;
            m_table->insert(region_id, sample);
        }
    }

    void Profile::disable(const std::string feature_name)
    {
        if (!m_is_enabled) {
           return;
        }

        throw geopm::Exception("Profile::disable()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::print(const std::string file_name, int depth)
    {
        if (!m_is_enabled) {
           return;
        }

        int is_done = 0;
        int is_all_done = 0;

        if (!m_shm_rank) {
            m_ctl_msg->app_status = GEOPM_STATUS_REPORT;
        }

        while (m_ctl_msg->ctl_status != GEOPM_STATUS_REPORT) {}
        geopm_pmpi_prof_enable(0);

        size_t buffer_offset = 0;
        char *buffer_ptr = (char *)(m_table_shmem->pointer());

        if (m_table_shmem->size() < file_name.length() + 1 + m_prof_name.length() + 1) {
            throw Exception("Profile:print() profile file name and profile name are too long to fit in a table buffer", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        strcpy(buffer_ptr, file_name.c_str());
        buffer_ptr += file_name.length() + 1;
        buffer_offset += file_name.length() + 1;
        strcpy(buffer_ptr, m_prof_name.c_str());
        buffer_offset += m_prof_name.length() + 1;
        while (!is_all_done) {
            is_done = m_table->name_fill(buffer_offset);
            PMPI_Allreduce(&is_done, &is_all_done, 1, MPI_INT, MPI_LAND, m_shm_comm);
            if (!m_shm_rank) {
                m_ctl_msg->app_status = GEOPM_STATUS_READY;
            }

            while (m_ctl_msg->ctl_status != GEOPM_STATUS_READY) {}
            PMPI_Barrier(m_shm_comm);
            if (!m_shm_rank && !is_all_done) {
                m_ctl_msg->app_status = GEOPM_STATUS_REPORT;
            }
            buffer_offset = 0;
        }
        if (!m_shm_rank) {
            m_ctl_msg->app_status = GEOPM_STATUS_SHUTDOWN;
        }
    }

    void Profile::init_cpu_list(void)
    {
        if (!m_is_enabled) {
           return;
        }

        int err = 0;
        unsigned int i = 0;

        hwloc_topology_t topology;
        hwloc_cpuset_t set;

        err = hwloc_topology_init(&topology);
        if (err) {
            throw Exception("Profile: unable to initialize hwloc", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        err = hwloc_topology_load(topology);
        if (err) {
            throw Exception("Profile: unable to load topology in hwloc", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        set = hwloc_bitmap_alloc();

        err = hwloc_get_cpubind(topology, set, HWLOC_CPUBIND_PROCESS);
        if (err) {
            if (!geopm_env_do_ignore_affinity()) {
                throw Exception("Profile: unable to get process binding from hwloc, set GEOPM_ERROR_AFFINITY_IGNORE to ignore error", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_cpu_list.push_front(0);
        }
        else {
            hwloc_bitmap_foreach_begin(i, set) {
                m_cpu_list.push_front(i);
            }
            hwloc_bitmap_foreach_end();
        }
        hwloc_bitmap_free(set);
        hwloc_topology_destroy(topology);
    }

    const struct geopm_prof_message_s GEOPM_INVALID_PROF_MSG = {-1, 0, {{0, 0}}, -1.0};

    ProfileSampler::ProfileSampler(const std::string shm_key, size_t table_size)
        : m_table_size(table_size)
        , m_do_report(false)
    {
        std::string key(shm_key);
        if (!key.size()) {
            key = geopm_env_shmkey();
        }
        if (!key.size()) {
            key = g_default_shmem_key;
        }
        key = key + "-sample";
        m_ctl_shmem = new SharedMemory(key, table_size);
        m_ctl_msg = (struct geopm_ctl_message_s *)m_ctl_shmem->pointer();
    }

    ProfileSampler::~ProfileSampler(void)
    {
        for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
            delete (*it);
        }
        delete m_ctl_shmem;
    }

    void ProfileSampler::initialize(void)
    {
        std::string shm_key;

        while (m_ctl_msg->app_status != GEOPM_STATUS_INITIALIZED) {}

        std::set<int> rank_set;
        for (int i = 0; i < GEOPM_MAX_NUM_CPU; i++) {
            if (m_ctl_msg->cpu_rank[i] >= 0) {
                (void) rank_set.insert(m_ctl_msg->cpu_rank[i]);
            }
        }

        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            shm_key = m_ctl_shmem->key() + "-" + std::to_string(*it);
            m_rank_sampler.push_front(new ProfileRankSampler(shm_key, m_table_size));
        }
        m_ctl_msg->ctl_status = GEOPM_STATUS_INITIALIZED;
        while (m_ctl_msg->app_status != GEOPM_STATUS_ACTIVE) {}
        m_ctl_msg->ctl_status = GEOPM_STATUS_ACTIVE;
    }

    void ProfileSampler::cpu_rank(std::vector<int> &cpu_rank)
    {
#ifdef _SC_NPROCESSORS_ONLN
        int num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
#else
        int num_cpu = 1;
        size_t len = sizeof(num_cpu);
        sysctl((int[2]) {CTL_HW, HW_NCPU}, 2, &num_cpu, &len, NULL, 0);
#endif
        cpu_rank.resize(num_cpu);
        if (num_cpu > GEOPM_MAX_NUM_CPU) {
            throw Exception("ProfileSampler::cpu_rank: Number of online CPUs is greater than GEOPM_MAX_NUM_CPU", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::copy(m_ctl_msg->cpu_rank, m_ctl_msg->cpu_rank + num_cpu, cpu_rank.begin());
    }

    size_t ProfileSampler::capacity(void)
    {
        size_t result = 0;
        for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
            result += (*it)->capacity();
        }
        return result;
    }

    void ProfileSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length)
    {
        int app_status = m_ctl_msg->app_status;
        length = 0;
        if (app_status == GEOPM_STATUS_ACTIVE ||
            app_status == GEOPM_STATUS_REPORT) {
            auto content_it = content.begin();
            for (auto rank_sampler_it = m_rank_sampler.begin();
                 rank_sampler_it != m_rank_sampler.end();
                 ++rank_sampler_it) {
                size_t rank_length = 0;
                (*rank_sampler_it)->sample(content_it, rank_length);
                content_it += rank_length;
                length += rank_length;
            }
            if (app_status == GEOPM_STATUS_REPORT) {
                region_names();
            }
        }
        else if (app_status != GEOPM_STATUS_SHUTDOWN) {
            throw Exception("ProfileSampler: invalid application status: " + std::to_string(app_status), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    bool ProfileSampler::do_shutdown(void)
    {
        return (m_ctl_msg->app_status == GEOPM_STATUS_SHUTDOWN);
    }

    bool ProfileSampler::do_report(void)
    {
        return m_do_report;
    }

    void ProfileSampler::region_names(void)
    {
        bool is_all_done = false;

        while (!is_all_done && m_ctl_msg->app_status != GEOPM_STATUS_SHUTDOWN) {
            m_ctl_msg->ctl_status = GEOPM_STATUS_REPORT;
            while (m_ctl_msg->app_status != GEOPM_STATUS_READY &&
                   m_ctl_msg->app_status != GEOPM_STATUS_SHUTDOWN) {}
            if (m_ctl_msg->app_status != GEOPM_STATUS_SHUTDOWN) {
                is_all_done = true;
                for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
                    if (!(*it)->name_fill(m_name_set)) {
                        is_all_done = false;
                    }
                }
            }
            m_ctl_msg->ctl_status = GEOPM_STATUS_READY;

            while (m_ctl_msg->app_status != GEOPM_STATUS_READY &&
                   m_ctl_msg->app_status != GEOPM_STATUS_SHUTDOWN) {}

            if (!is_all_done && m_ctl_msg->app_status == GEOPM_STATUS_SHUTDOWN) {
                throw Exception("ProfileSampler::report(): Application shutdown while report was being generated", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_rank_sampler.front()->report_name(m_report_name);
        m_rank_sampler.front()->profile_name(m_profile_name);
        m_do_report = true;

        while (m_ctl_msg->app_status != GEOPM_STATUS_SHUTDOWN) {}
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

    ProfileRankSampler::ProfileRankSampler(const std::string shm_key, size_t table_size)
        : m_table_shmem(SharedMemory(shm_key, table_size))
        , m_table(ProfileTable(m_table_shmem.size(), m_table_shmem.pointer()))
        , m_region_entry(GEOPM_INVALID_PROF_MSG)
        , m_is_name_finished(false)
    {

    }

    ProfileRankSampler::~ProfileRankSampler()
    {

    }

    size_t ProfileRankSampler::capacity(void)
    {
        return m_table.capacity();
    }

    void ProfileRankSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content_begin, size_t &length)
    {
        m_table.dump(content_begin, length);
        std::sort(content_begin, content_begin + length, geopm_prof_compare);

    }

    bool ProfileRankSampler::name_fill(std::set<std::string> &name_set)
    {
        size_t header_offset = 0;

        if (!m_is_name_finished) {
            if (name_set.empty()) {
                m_report_name = (char *)m_table_shmem.pointer();
                header_offset += m_report_name.length() + 1;
                m_prof_name = (char *)m_table_shmem.pointer() + header_offset;
                header_offset += m_prof_name.length() + 1;
            }
            m_is_name_finished = m_table.name_set(header_offset, name_set);
        }

        return m_is_name_finished;
    }

    void ProfileRankSampler::report(std::ofstream &file_stream)
    {
        char hostname[NAME_MAX];

        gethostname(hostname, NAME_MAX);
        if (!file_stream.is_open()) {
            file_stream.open(m_report_name + "-" + std::string(hostname), std::ios_base::out);
            file_stream << "Profile: " << m_prof_name << std::endl;
        }

        file_stream << std::endl << "Rank " << m_region_entry.rank << " Report:" << std::endl;

        for (auto it = m_name_set.begin(); it != m_name_set.end(); ++it) {
            uint64_t region_id = geopm_crc32_str(0, (*it).c_str());
            file_stream << "Region " + (*it) + ":" << std::endl;

            auto entry = m_agg_stats.find(region_id);
            if (entry == m_agg_stats.end()) {
                file_stream << "\truntime: " << 0.0 << std::endl;
            }
            else {
                file_stream << "\truntime: " << (*entry).second.signal[GEOPM_SAMPLE_TYPE_RUNTIME] << std::endl;
            }
        }
        // Report mpi
        auto entry = m_agg_stats.find(GEOPM_REGION_ID_MPI);
        if (entry != m_agg_stats.end()) {
            file_stream << "Region mpi-sync:" << std::endl;
            file_stream << "\truntime: " << (*entry).second.signal[GEOPM_SAMPLE_TYPE_RUNTIME] << std::endl;
        }
        // Report outer loop
        entry = m_agg_stats.find(GEOPM_REGION_ID_OUTER);
        if (entry != m_agg_stats.end()) {
            file_stream << "Region outer-sync:" << std::endl;
            file_stream << "\truntime: " << (*entry).second.signal[GEOPM_SAMPLE_TYPE_RUNTIME] << std::endl;
        }
    }

    void ProfileRankSampler::report_name(std::string &report_str)
    {
        report_str = m_report_name;
    }

    void ProfileRankSampler::profile_name(std::string &prof_str)
    {
        prof_str = m_prof_name;
    }

    ProfileTable::ProfileTable(size_t size, void *buffer)
        : LockingHashTable(size, buffer)
    {

    }

    ProfileTable::~ProfileTable()
    {

    }


    bool ProfileTable::sticky(const struct geopm_prof_message_s &value)
    {
        bool result = false;
        if (value.progress == 0.0 || value.progress == 1.0) {
            result = true;
        }
        return result;
    }
}
