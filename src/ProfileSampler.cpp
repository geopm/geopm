/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ProfileSampler.hpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <iostream>
#include <sstream>

#include <float.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "geopm_time.h"
#include "geopm_sched.h"
#include "Environment.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "ProfileTable.hpp"
#include "Comm.hpp"
#include "ControlMessage.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "ApplicationSampler.hpp"
#include "config.h"

namespace geopm
{
    ProfileSamplerImp::ProfileSamplerImp(size_t table_size)
        : ProfileSamplerImp(platform_topo(), table_size)
    {
    }

    ProfileSamplerImp::ProfileSamplerImp(const PlatformTopo &topo, size_t table_size)
        : m_ctl_shmem(nullptr)
        , m_ctl_msg(nullptr)
        , m_table_size(table_size)
        , m_do_report(false)
        , m_rank_per_node(0)
    {
        const Environment &env = environment();
        const std::string key_base = ApplicationSampler::default_shmkey();
        std::string sample_key = key_base + "-sample";
        std::string sample_key_path("/dev/shm/" + sample_key);
        // Remove shared memory file if one already exists.
        (void)unlink(sample_key_path.c_str());
        m_ctl_shmem = SharedMemory::make_unique_owner(sample_key, sizeof(struct geopm_ctl_message_s));
        m_ctl_msg = geopm::make_unique<ControlMessageImp>(*(struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), true, true, env.timeout());

        errno = 0; // Ignore errors from the unlink calls.
    }

    ProfileSamplerImp::~ProfileSamplerImp()
    {
        if (m_ctl_shmem) {
            m_ctl_shmem->unlink();
        }
    }

    void ProfileSamplerImp::initialize(void)
    {
        std::ostringstream shm_key;

        m_ctl_msg->wait(); // M_STATUS_MAP_BEGIN
        m_ctl_msg->step(); // M_STATUS_MAP_BEGIN
        m_ctl_msg->wait(); // M_STATUS_MAP_END

        std::set<int> rank_set;
        for (int i = 0; i < GEOPM_MAX_NUM_CPU; i++) {
            if (m_ctl_msg->cpu_rank(i) >= 0) {
                (void) rank_set.insert(m_ctl_msg->cpu_rank(i));
            }
        }

        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            shm_key.str("");
            shm_key << m_ctl_shmem->key() <<  "-"  << *it;
            m_rank_sampler.push_front(geopm::make_unique<ProfileRankSamplerImp>(shm_key.str(), m_table_size));
        }
        m_rank_per_node = rank_set.size();
        if (m_rank_per_node == 0) {
            m_ctl_msg->abort();
            throw Exception("ProfileSamplerImp::initialize(): Application ranks were not listed as running on any CPUs.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        m_ctl_msg->step(); // M_STATUS_MAP_END
    }

    void ProfileSamplerImp::controller_ready(void)
    {
        m_ctl_msg->wait();  // M_STATUS_SAMPLE_BEGIN
        m_ctl_msg->step();  // M_STATUS_SAMPLE_BEGIN
    }

    int ProfileSamplerImp::rank_per_node(void) const
    {
        return m_rank_per_node;
    }

    std::vector<int> ProfileSamplerImp::cpu_rank(void) const
    {
        uint32_t num_cpu = geopm_sched_num_cpu();
        std::vector<int> result(num_cpu);
        if (num_cpu > GEOPM_MAX_NUM_CPU) {
            throw Exception("ProfileSamplerImp::cpu_rank: Number of online CPUs is greater than GEOPM_MAX_NUM_CPU",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (unsigned cpu = 0; cpu < num_cpu; ++cpu) {
            result[cpu] = m_ctl_msg->cpu_rank(cpu);
        }
        return result;
    }

    void ProfileSamplerImp::check_sample_end(void)
    {
        if (m_ctl_msg->is_sample_end()) {  // M_STATUS_SAMPLE_END
            //comm->barrier();  // TODO: is this needed?
            m_ctl_msg->step();
            while (!m_ctl_msg->is_name_begin() &&
                   !m_ctl_msg->is_shutdown()) {

            }
            if (m_ctl_msg->is_name_begin()) {  // M_STATUS_NAME_BEGIN
                region_names();
            }
        }
    }

    bool ProfileSamplerImp::do_shutdown(void) const
    {
        return m_ctl_msg->is_shutdown();
    }

    bool ProfileSamplerImp::do_report(void) const
    {
        return m_do_report;
    }

    void ProfileSamplerImp::region_names(void)
    {
        m_ctl_msg->step();  // M_STATUS_NAME_BEGIN

        bool is_all_done = false;
        while (!is_all_done) {
            m_ctl_msg->loop_begin();  // M_STATUS_NAME_LOOP_BEGIN
            m_ctl_msg->wait();        // M_STATUS_NAME_LOOP_END
            is_all_done = true;
            for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
                if (!(*it)->name_fill(m_name_set)) {
                    is_all_done = false;
                }
            }
            m_ctl_msg->step();  // M_STATUS_NAME_LOOP_END
            if (!is_all_done && m_ctl_msg->is_shutdown()) {
                throw Exception("ProfileSamplerImp::region_names(): Application shutdown while report was being generated", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_rank_sampler.front()->report_name(m_report_name);
        m_rank_sampler.front()->profile_name(m_profile_name);

        m_do_report = true;

        m_ctl_msg->wait();  // M_STATUS_NAME_END
        m_ctl_msg->step();  // M_STATUS_NAME_END
        m_ctl_msg->wait();  // M_STATUS_SHUTDOWN
    }

    std::set<std::string> ProfileSamplerImp::name_set(void) const
    {
        return m_name_set;
    }

    std::string ProfileSamplerImp::report_name(void) const
    {
        return m_report_name;
    }

    std::string ProfileSamplerImp::profile_name(void) const
    {
        return m_profile_name;
    }

    void ProfileSamplerImp::abort(void)
    {
        m_ctl_msg->abort();
    }

    ProfileRankSamplerImp::ProfileRankSamplerImp(const std::string &shm_key, size_t table_size)
        : m_table_shmem(nullptr)
        , m_table(nullptr)
        , m_is_name_finished(false)
    {
        std::string key_path("/dev/shm/" + shm_key);
        (void)unlink(key_path.c_str());
        errno = 0; // Ignore errors from the unlink call.
        m_table_shmem = SharedMemory::make_unique_owner(shm_key, table_size);
        m_table = geopm::make_unique<ProfileTableImp>(m_table_shmem->size(), m_table_shmem->pointer());
    }

    ProfileRankSamplerImp::~ProfileRankSamplerImp()
    {
        if (m_table_shmem) {
            m_table_shmem->unlink();
        }
    }

    bool ProfileRankSamplerImp::name_fill(std::set<std::string> &name_set)
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

    void ProfileRankSamplerImp::report_name(std::string &report_str) const
    {
        report_str = m_report_name;
    }

    void ProfileRankSamplerImp::profile_name(std::string &prof_str) const
    {
        prof_str = m_prof_name;
    }
}
