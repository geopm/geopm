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
#include "ControlMessage.hpp"
#include "SharedMemory.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"
#include "Exception.hpp"
#include "Comm.hpp"
#include "Helper.hpp"
#include "geopm_debug.hpp"

namespace geopm
{

    int Profile::get_cpu(void)
    {
        static thread_local int result = -1;
        if (result == -1) {
            result = geopm_sched_get_cpu();
#ifdef GEOPM_DEBUG
            if (result >= geopm_sched_num_cpu()) {
                throw geopm::Exception("Profile::get_cpu(): Number of online CPUs is less than or equal to the value returned by sched_getcpu()",
                                       GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
        }
        return result;
    }

    ProfileImp::ProfileImp(const std::string &prof_name,
                           const std::string &key_base,
                           const std::string &report,
                           double timeout,
                           std::shared_ptr<Comm> comm,
                           std::shared_ptr<ControlMessage> ctl_msg,
                           int num_cpu,
                           std::set<int> cpu_set,
                           std::shared_ptr<ProfileTable> table,
                           std::shared_ptr<Comm> reduce_comm,
                           std::shared_ptr<ApplicationStatus> app_status,
                           std::shared_ptr<ApplicationRecordLog> app_record_log)
        : m_is_enabled(false)
        , m_prof_name(prof_name)
        , m_key_base(key_base)
        , m_report(report)
        , m_timeout(timeout)
        , m_comm(comm)
        , m_curr_region_id(0)
        , m_current_hash(GEOPM_REGION_HASH_UNMARKED)
        , m_ctl_shmem(nullptr)
        , m_ctl_msg(ctl_msg)
        , m_num_cpu(num_cpu)
        , m_cpu_set(cpu_set)
        , m_table_shmem(nullptr)
        , m_table(table)
        , m_shm_comm(nullptr)
        , m_process(-1)
        , m_shm_rank(0)
        , m_reduce_comm(reduce_comm)
        , m_app_status(app_status)
        , m_app_record_log(app_record_log)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
    {

    }

    ProfileImp::ProfileImp()
        : ProfileImp(environment().profile(),
                     environment().shmkey(),
                     environment().report(),
                     environment().timeout(),
                     nullptr,  // comm
                     nullptr,  // ctl_msg
                     platform_topo().num_domain(GEOPM_DOMAIN_CPU),
                     {},  // cpu_set
                     nullptr,  // table
                     nullptr,  // reduce_comm
                     nullptr,  // app_status
                     nullptr)  // app_record_log
    {

    }

    void ProfileImp::init(void)
    {
        if (m_is_enabled) {
            return;
        }
        if (m_comm == nullptr) {
            m_comm = Comm::make_unique();
        }
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
        if (m_reduce_comm == nullptr) {
            m_reduce_comm = Comm::make_unique();
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
        std::string sample_key(m_key_base + "-sample");
        int shm_num_rank = 0;

        init_prof_comm(std::move(m_comm), shm_num_rank);
        std::string step;
        try {
            step = "ctl_msg";
            init_ctl_msg(sample_key);
            step = "cpu_set";
            init_cpu_set(m_num_cpu);
            step = "cpu_affinity";
            init_cpu_affinity(shm_num_rank);
            step = "table";
            init_table(sample_key);
            step = "app_status";
            init_app_status();
            step = "app_record_log";
            init_app_record_log();
            m_is_enabled = true;
        }
        catch (const Exception &ex) {
            if (m_process < 0) {
                std::cerr << "Warning: <geopm> Controller handshake failed at step "
                          << step << ", running without geopm." << std::endl;
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

#ifdef GEOPM_DEBUG
        // assert that all objects were created
        if (m_is_enabled) {
            std::vector<std::string> null_objects;
            if (!m_ctl_msg) {
                null_objects.push_back("m_ctl_msg");
            }
            if (!m_table) {
                null_objects.push_back("m_table");
            }
            if (!null_objects.empty()) {
                std::string objs = string_join(null_objects, ", ");
                throw Exception("Profile::init(): one or more internal objects not initialized: " + objs,
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
#endif

    }

    // TODO: m_comm is never used again after this
    void ProfileImp::init_prof_comm(std::shared_ptr<Comm> comm, int &shm_num_rank)
    {
        if (!m_shm_comm) {
            m_process = comm->rank();
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
            m_ctl_shmem = SharedMemory::make_unique_user(sample_key, m_timeout);
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

    void ProfileImp::init_cpu_set(int num_cpu)
    {
        if (m_cpu_set.empty()) {
            cpu_set_t *proc_cpuset = NULL;
            proc_cpuset = CPU_ALLOC(num_cpu);
            if (!proc_cpuset) {
                throw Exception("ProfileImp: unable to allocate process CPU mask",
                                ENOMEM, __FILE__, __LINE__);
            }
            geopm_sched_proc_cpuset(num_cpu, proc_cpuset);
            for (int ii = 0; ii < num_cpu; ++ii) {
                if (CPU_ISSET(ii, proc_cpuset)) {
                    m_cpu_set.insert(ii);
                }
            }
            free(proc_cpuset);
        }
    }

    void ProfileImp::init_cpu_affinity(int shm_num_rank)
    {
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_MAP_BEGIN
        m_ctl_msg->wait();  // M_STATUS_MAP_BEGIN

        // Assign ranks to cores; -1 indicates unassigned and -2 indicates double assigned.
        for (int ii = 0 ; ii < shm_num_rank; ++ii) {
            if (ii == m_shm_rank) {
                if (ii == 0) {
                    for (int jj = 0; jj < GEOPM_MAX_NUM_CPU; ++jj) {
                        m_ctl_msg->cpu_rank(jj, -1);
                    }
                    for (auto it = m_cpu_set.begin(); it != m_cpu_set.end(); ++it) {
                        m_ctl_msg->cpu_rank(*it, m_process);
                    }
                }
                else {
                    for (auto it = m_cpu_set.begin(); it != m_cpu_set.end(); ++it) {
                        if (m_ctl_msg->cpu_rank(*it) != -1) {
                            m_ctl_msg->cpu_rank(*it, -2);
                        }
                        else {
                            m_ctl_msg->cpu_rank(*it, m_process);
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

    void ProfileImp::init_table(const std::string &sample_key)
    {
        if (!m_table) {
            std::string table_shm_key(sample_key);
            table_shm_key += "-" + std::to_string(m_process);
            m_table_shmem = SharedMemory::make_unique_user(table_shm_key, m_timeout);
            m_table_shmem->unlink();
            m_table = geopm::make_unique<ProfileTableImp>(m_table_shmem->size(), m_table_shmem->pointer());
        }

        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_SAMPLE_BEGIN
        m_ctl_msg->wait();  // M_STATUS_SAMPLE_BEGIN
    }

    void ProfileImp::init_app_status(void)
    {
        if (m_app_status == nullptr) {
            m_shm_comm->barrier();
            std::string key = m_key_base + "-status";
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(key, m_timeout);
            m_app_status = ApplicationStatus::make_unique(m_num_cpu, shmem);
            // wait until all ranks attach, then unlink
            m_shm_comm->barrier();
            if (!m_shm_rank) {
                shmem->unlink();
            }
        }
        GEOPM_DEBUG_ASSERT(m_app_status != nullptr,
                           "Profile::init(): m_app_status not initialized");
    }

    void ProfileImp::init_app_record_log(void)
    {
        if (m_process < 0) {
            throw Exception("Profile::init(): invalid process",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (m_app_record_log == nullptr) {
            std::string key = m_key_base + "-record-log-" + std::to_string(m_process);
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(key, m_timeout);
            m_app_record_log = ApplicationRecordLog::make_unique(shmem);
            shmem->unlink();
        }

        GEOPM_DEBUG_ASSERT(m_app_record_log != nullptr,
                           "Profile::init(): m_app_record_log not initialized");
        GEOPM_DEBUG_ASSERT(m_process >= 0,
                           "Profile::init(): m_process not initialized");

        m_app_status->set_process(m_cpu_set, m_process);

        m_app_record_log->set_process(m_process);
        geopm_time_s start_time;
        geopm_time_zero(&start_time);
        m_app_record_log->set_time_zero(start_time);
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

        send_names(m_report);
        m_shm_comm->barrier();
        m_ctl_msg->step();  // M_STATUS_SHUTDOWN
        m_shm_comm->tear_down();
        m_shm_comm.reset();
        m_is_enabled = false;
    }

    uint64_t ProfileImp::region(const std::string &region_name, long hint)
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

        uint64_t hash = geopm_region_id_hash(region_id);
        uint64_t hint = geopm_region_id_hint(region_id);

        if (m_current_hash == GEOPM_REGION_HASH_UNMARKED) {
            // not currently in a region; enter region
            m_current_hash = hash;
            geopm_time_s now;
            geopm_time(&now);
            m_app_record_log->enter(hash, now);
            for (const int &cpu_idx : m_cpu_set) {
                m_app_status->set_hash(cpu_idx, hash, hint);
            }
        }
        else {
            // top level and nested entries inside a region both update hints
            set_hint(hint);
        }
        m_hint_stack.push(hint);

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

        uint64_t hash = geopm_region_id_hash(region_id);
        geopm_time_s now;
        geopm_time(&now);

        if (m_hint_stack.empty()) {
            throw Exception("Profile::exit(): expected at least one enter before exit call",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        m_hint_stack.pop();
        if (m_hint_stack.empty()) {
            // leaving outermost region, clear hints and exit region
            m_app_record_log->exit(hash, now);
            m_current_hash = GEOPM_REGION_HASH_UNMARKED;
            // reset both progress ints; calling post() outside of
            // region is an error
            for (auto cpu : m_cpu_set) {
                // Note: does not use thread_init() because the region
                // hash has been cleared first.  This prevents thread
                // progress from decreasing at the end of a region.
                // The thread progress value is not valid outside of a
                // region.
                m_app_status->set_hash(cpu, m_current_hash, GEOPM_REGION_HINT_UNSET);
                m_app_status->reset_work_units(cpu);
            }
        }
        else {
            // still nested, restore previous hint
            auto hint = m_hint_stack.top();
            set_hint(hint);
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

        geopm_time_s now;
        geopm_time(&now);
        m_app_record_log->epoch(now);

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif

    }

    void ProfileImp::thread_init(uint32_t num_work_unit)
    {
        // Ignore calls with num_work_unit set to 1: work cannot be
        // shared between threads.
        if (!m_is_enabled || num_work_unit <= 1) {
            return;
        }

        for (const auto &cpu : m_cpu_set) {
            m_app_status->set_total_work_units(cpu, num_work_unit);
        }
    }

    void ProfileImp::thread_post(int cpu)
    {
        if (!m_is_enabled) {
            return;
        }

        m_app_status->increment_work_unit(cpu);
    }

    void ProfileImp::send_names(const std::string &report_file_name)
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

        if (m_table_shmem->size() < report_file_name.length() + 1 + m_prof_name.length() + 1) {
            throw Exception("ProfileImp::send_names() profile file name and profile name are too long to fit in a table buffer", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        strncpy(buffer_ptr, report_file_name.c_str(), buffer_remain - 1);
        buffer_ptr += report_file_name.length() + 1;
        buffer_offset += report_file_name.length() + 1;
        buffer_remain -= report_file_name.length() + 1;
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

        if (m_process == 0) {
            std::cout << "GEOPM startup (seconds):  " << max_overhead[0] << std::endl;
            std::cout << "GEOPM runtime (seconds):  " << max_overhead[1] << std::endl;
            std::cout << "GEOPM shutdown (seconds): " << max_overhead[2] << std::endl;
        }
#endif

    }

    void ProfileImp::enable_pmpi(void)
    {
        // only implemented by DefaultProfile singleton
    }

    void ProfileImp::set_hint(uint64_t hint)
    {
        for (auto cpu : m_cpu_set) {
            m_app_status->set_hint(cpu, hint);
        }
    }
}
