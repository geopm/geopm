/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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

#include "geopm_hint.h"
#include "geopm_hash.h"
#include "geopm_time.h"
#include "geopm_sched.h"
#include "Environment.hpp"
#include "geopm/PlatformTopo.hpp"
#include "ProfileTable.hpp"
#include "geopm/SharedMemory.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"
#include "ApplicationSampler.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"
#include "geopm_shmem.h"


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
                           int num_cpu,
                           std::set<int> cpu_set,
                           std::shared_ptr<ProfileTable> table,
                           std::shared_ptr<ApplicationStatus> app_status,
                           std::shared_ptr<ApplicationRecordLog> app_record_log,
                           bool do_profile)
        : m_is_enabled(false)
        , m_prof_name(prof_name)
        , m_report(report)
        , m_curr_region_id(0)
        , m_current_hash(GEOPM_REGION_HASH_UNMARKED)
        , m_ctl_shmem(nullptr)
        , m_ctl_msg(ctl_msg)
        , m_num_cpu(num_cpu)
        , m_cpu_set(cpu_set)
        , m_table_shmem(nullptr)
        , m_table(table)
        , m_shm_rank(0)
        , m_app_status(app_status)
        , m_app_record_log(app_record_log)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
        , m_do_profile(do_profile)
    {

    }

    ProfileImp::ProfileImp()
        : ProfileImp(environment().profile(),
                     shmem_path_prof("", getpid(), geteuid())
                     environment().report(),
                     environment().timeout(),
                     nullptr,  // ctl_msg
                     platform_topo().num_domain(GEOPM_DOMAIN_CPU),
                     {},  // cpu_set
                     nullptr,  // table
                     nullptr,  // app_status
                     nullptr,  // app_record_log
                     environment().timeout() != -1)
    {

    }

    void ProfileImp::init(void)
    {
        if (m_is_enabled || !m_do_profile) {
            return;
        }
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#else
        /// read and write to satisfy clang ifndef GEOPM_OVERHEAD
        ++m_overhead_time;
        --m_overhead_time;
        ++m_overhead_time_startup;
        --m_overhead_time_startup;
        ++m_overhead_time_shutdown;
        --m_overhead_time_shutdown;
#endif
        std::string sample_key = shmem_path_prof("sample", getpid(), geteuid());
        int shm_num_rank = 0;

        std::string step;
        try {
            step = "table";
            init_table(sample_key);
            step = "app_status";
            init_app_status();
            step = "app_record_log";
            init_app_record_log();
            m_is_enabled = true;
        }
        catch (const Exception &ex) {
            std::cerr << "Warning: <geopm> Controller handshake failed at step "
                      << step << ", running without geopm." << std::endl;
            int err = ex.err_value();
            if (err != GEOPM_ERROR_RUNTIME) {
                char tmp_msg[NAME_MAX];
                geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
                tmp_msg[NAME_MAX-1] = '\0';
                std::cerr << tmp_msg << std::endl;
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

    void ProfileImp::init_cpu_set(int num_cpu)
    {
        if (m_cpu_set.empty()) {
            cpu_set_t *proc_cpuset = NULL;
            proc_cpuset = CPU_ALLOC(num_cpu);
            if (proc_cpuset == nullptr) {
                throw Exception("ProfileImp: unable to allocate process CPU mask",
                                ENOMEM, __FILE__, __LINE__);
            }
            geopm_sched_proc_cpuset(num_cpu, proc_cpuset);
            for (int ii = 0; ii < num_cpu; ++ii) {
                if (CPU_ISSET(ii, proc_cpuset)) {
                    m_cpu_set.insert(ii);
                }
            }
            CPU_FREE(proc_cpuset);
        }
    }

    void ProfileImp::init_table(const std::string &sample_key)
    {
        if (!m_table) {
            m_table_shmem = SharedMemory::make_unique_user(sample_key, 0);
            m_table = geopm::make_unique<ProfileTableImp>(m_table_shmem->size(), m_table_shmem->pointer());
        }
    }

    void ProfileImp::init_app_status(void)
    {
        if (m_app_status == nullptr) {
            std::string key = m_key_base + "-status";
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(key, 0);
            m_app_status = ApplicationStatus::make_unique(m_num_cpu, shmem);
            // wait until all ranks attach, then unlink
            if (!m_shm_rank) {
                shmem->unlink();
            }
        }
        GEOPM_DEBUG_ASSERT(m_app_status != nullptr,
                           "Profile::init(): m_app_status not initialized");
    }

    void ProfileImp::init_app_record_log(void)
    {
        if (m_app_record_log == nullptr) {
            std::string key = m_key_base + "-record-log";
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(key, 0);
            m_app_record_log = ApplicationRecordLog::make_unique(shmem);
            shmem->unlink();
        }

        GEOPM_DEBUG_ASSERT(m_app_record_log != nullptr,
                           "Profile::init(): m_app_record_log not initialized");

        m_app_status->set_valid_cpu(m_cpu_set, true);

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

#ifdef GEOPM_OVERHEAD
        m_overhead_time_shutdown = geopm_time_since(&overhead_entry);
#endif

        send_names(m_report);
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

        geopm::check_hint(hint);
        uint64_t result = m_table->key(region_name);

#ifdef GEOPM_DEBUG
        m_region_ids.emplace(result);
#endif

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
        geopm::check_hint(hint);

#ifdef GEOPM_DEBUG
        if (hash != GEOPM_REGION_HASH_UNMARKED &&
            m_region_ids.find(hash) == m_region_ids.end()) {
            throw Exception("Profile::region(): Region '" + geopm::string_format_hex(hash) +
                            "' has not yet been created.  Call geopm_prof_region() first.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
#endif

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
        // TODO: Rewrite with pipe
        if (!m_is_enabled || !m_table_shmem) {
            return;
        }

#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif

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
        is_done = m_table->name_fill(buffer_offset);
        if (!is_done) {
            throw Exception("ProfileTable not big enough for all names",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
        std::cerr << "Info: <geopm> Overhead (seconds) PID: " << getpid()
                  << " startup:  " << m_overhead_time_startup <<
                  << " runtime:  " << m_overhead_time <<
                  << " shutdown: " << m_overhead_time_shutdown << std::endl;
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
