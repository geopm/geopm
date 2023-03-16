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
#include "geopm/SharedMemory.hpp"
#include "ApplicationRecordLog.hpp"
#include "ApplicationStatus.hpp"
#include "ApplicationSampler.hpp"
#include "Scheduler.hpp"
#include "geopm/ServiceProxy.hpp"
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
                           const std::string &report,
                           int num_cpu,
                           std::set<int> cpu_set,
                           std::shared_ptr<ApplicationStatus> app_status,
                           std::shared_ptr<ApplicationRecordLog> app_record_log,
                           bool do_profile)
        : m_is_enabled(false)
        , m_prof_name(prof_name)
        , m_report(report)
        , m_curr_region_id(0)
        , m_current_hash(GEOPM_REGION_HASH_UNMARKED)
        , m_ctl_shmem(nullptr)
        , m_num_cpu(num_cpu)
        , m_cpu_set(cpu_set)
        , m_app_status(app_status)
        , m_app_record_log(app_record_log)
        , m_overhead_time(0.0)
        , m_overhead_time_startup(0.0)
        , m_overhead_time_shutdown(0.0)
        , m_do_profile(do_profile)
    {
        if (!m_do_profile) {
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
        init_cpu_set(m_num_cpu);
        try {
            // TODO: Add a constructor argument to enable mocking
            auto service_proxy = ServiceProxy::make_unique();
            service_proxy->platform_start_profile(m_prof_name);
            init_app_status();
            init_app_record_log();
        }
        catch (const Exception &ex) {
            std::cerr << "Warning: <geopm> Failed to connect with geopmd, running without geopm. "
                      << "Error: " << ex.what() << "." << std::endl;
            int err = ex.err_value();
            char tmp_msg[PATH_MAX];
            geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
            tmp_msg[PATH_MAX-1] = '\0';
            std::cerr << tmp_msg << std::endl;
        }
        m_is_enabled = true;
#ifdef GEOPM_OVERHEAD
        m_overhead_time_startup = geopm_time_since(&overhead_entry);
#endif
    }

    ProfileImp::ProfileImp()
        : ProfileImp(environment().profile(),
                     environment().report(),
                     platform_topo().num_domain(GEOPM_DOMAIN_CPU),
                     {},  // cpu_set
                     nullptr,  // app_status
                     nullptr,  // app_record_log
                     environment().timeout() != -1)
    {

    }

    void ProfileImp::init_cpu_set(int num_cpu)
    {
        if (m_cpu_set.empty()) {
            auto proc_cpuset = geopm::make_cpu_set(num_cpu, {});
            geopm_sched_proc_cpuset(num_cpu, proc_cpuset.get());
            for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
                if (CPU_ISSET(cpu_idx, proc_cpuset)) {
                    m_cpu_set.insert(cpu_idx);
                }
            }
        }
    }

    void ProfileImp::init_app_status(void)
    {
        if (m_app_status == nullptr) {
            std::string shmem_path = shmem_path_prof("status", getpid(), geteuid());
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(shmem_path, 0);
            m_app_status = ApplicationStatus::make_unique(m_num_cpu, shmem);
        }
        GEOPM_DEBUG_ASSERT(m_app_status != nullptr,
                           "Profile::init_app_status(): m_app_status not initialized");
    }

    void ProfileImp::init_app_record_log(void)
    {
        if (m_app_record_log == nullptr) {
            std::string shmem_path = shmem_path_prof("record-log", getpid(), geteuid());
            std::shared_ptr<SharedMemory> shmem = SharedMemory::make_unique_user(shmem_path, 0);
            m_app_record_log = ApplicationRecordLog::make_unique(shmem);
        }

        GEOPM_DEBUG_ASSERT(m_app_record_log != nullptr,
                           "Profile::init_app_record_log(): m_app_record_log not initialized");

        m_app_status->set_valid_cpu(m_cpu_set, true);

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
        auto service_proxy = ServiceProxy::make_unique();
        service_proxy->platform_stop_profile(region_names());
#ifdef GEOPM_OVERHEAD
        std::cerr << "Info: <geopm> Overhead (seconds) PID: " << getpid()
                  << " startup:  " << m_overhead_time_startup <<
                  << " runtime:  " << m_overhead_time <<
                  << " shutdown: " << m_overhead_time_shutdown << std::endl;
#endif
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
        uint64_t result = 0;
        auto name_it = m_region_names.lower_bound(region_name);
        if (name_it == m_region_names.end() || name_it->first != region_name) {
            result = geopm_crc32_str(region_name.c_str());
#ifdef GEOPM_DEBUG
            m_region_ids.insert(result);
#endif
            /// Record hint when registering a region.
            result = geopm_region_id_set_hint(hint, result);
            m_region_names.emplace_hint(name_it, region_name, result);
        }
        else {
            result = name_it->second;
        }

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

    std::vector<std::string> ProfileImp::region_names(void)
    {
#ifdef GEOPM_OVERHEAD
        struct geopm_time_s overhead_entry;
        geopm_time(&overhead_entry);
#endif
        std::vector<std::string> result;
        for (const auto &it : m_region_names) {
            result.push_back(it.first);
        }

#ifdef GEOPM_OVERHEAD
        m_overhead_time += geopm_time_since(&overhead_entry);
#endif
        return result;
    }

    void ProfileImp::set_hint(uint64_t hint)
    {
        for (auto cpu : m_cpu_set) {
            m_app_status->set_hint(cpu, hint);
        }
    }
}
