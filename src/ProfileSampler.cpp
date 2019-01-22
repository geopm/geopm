/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include "geopm_internal.h"
#include "geopm_time.h"
#include "geopm_signal_handler.h"
#include "geopm_sched.h"
#include "geopm_env.h"
#include "Helper.hpp"
#include "PlatformTopo.hpp"
#include "ProfileSampler.hpp"
#include "ProfileTable.hpp"
#include "ProfileThread.hpp"
#include "SampleScheduler.hpp"
#include "Comm.hpp"
#include "ControlMessage.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "config.h"

static bool geopm_prof_compare(const std::pair<uint64_t, struct geopm_prof_message_s> &aa, const std::pair<uint64_t, struct geopm_prof_message_s> &bb)
{
    return geopm_time_comp(&(aa.second.timestamp), &(bb.second.timestamp));
}

namespace geopm
{
    const struct geopm_prof_message_s GEOPM_INVALID_PROF_MSG = {-1, 0, GEOPM_TIME_REF, -1.0};

    ProfileSampler::ProfileSampler(size_t table_size)
        : ProfileSampler(platform_topo(), table_size)
    {
    }

    ProfileSampler::ProfileSampler(IPlatformTopo &topo, size_t table_size)
        : m_ctl_shmem(nullptr)
        , m_ctl_msg(nullptr)
        , m_table_size(table_size)
        , m_do_report(false)
        , m_tprof_shmem(nullptr)
        , m_tprof_table(nullptr)
        , m_rank_per_node(0)
    {
        std::string sample_key(geopm_env_shmkey());
        sample_key += "-sample";
        std::string sample_key_path("/dev/shm/" + sample_key);
        // Remove shared memory file if one already exists.
        (void)unlink(sample_key_path.c_str());
        m_ctl_shmem = geopm::make_unique<SharedMemory>(sample_key, sizeof(struct geopm_ctl_message_s));
        m_ctl_msg = geopm::make_unique<ControlMessage>(*(struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), true, true);

        std::string tprof_key(geopm_env_shmkey());
        tprof_key += "-tprof";
        std::string tprof_key_path("/dev/shm/" + tprof_key);
        // Remove shared memory file if one already exists.
        (void)unlink(tprof_key_path.c_str());
        size_t tprof_size = 64 * topo.num_domain(IPlatformTopo::M_DOMAIN_CPU);
        m_tprof_shmem = geopm::make_unique<SharedMemory>(tprof_key, tprof_size);
        m_tprof_table = geopm::make_unique<ProfileThreadTable>(tprof_size, m_tprof_shmem->pointer());
        errno = 0; // Ignore errors from the unlink calls.
    }

    ProfileSampler::~ProfileSampler() = default;

    void ProfileSampler::initialize(void)
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
            m_rank_sampler.push_front(geopm::make_unique<ProfileRankSampler>(shm_key.str(), m_table_size));
        }
        m_rank_per_node = rank_set.size();
        if (m_rank_per_node == 0) {
            m_ctl_msg->abort();
            throw Exception("ProfileSampler::initialize(): Application ranks were not listed as running on any CPUs.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        m_ctl_msg->step(); // M_STATUS_MAP_END
    }

    void ProfileSampler::controller_ready(void)
    {
        m_ctl_msg->wait();  // M_STATUS_SAMPLE_BEGIN
        m_ctl_msg->step();  // M_STATUS_SAMPLE_BEGIN
    }

    int ProfileSampler::rank_per_node(void) const
    {
        return m_rank_per_node;
    }

    std::vector<int> ProfileSampler::cpu_rank(void) const
    {
        uint32_t num_cpu = geopm_sched_num_cpu();
        std::vector<int> result(num_cpu);
        if (num_cpu > GEOPM_MAX_NUM_CPU) {
            throw Exception("ProfileSampler::cpu_rank: Number of online CPUs is greater than GEOPM_MAX_NUM_CPU",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (unsigned cpu = 0; cpu < num_cpu; ++cpu) {
            result[cpu] = m_ctl_msg->cpu_rank(cpu);
        }
        return result;
    }

    size_t ProfileSampler::capacity(void) const
    {
        size_t result = 0;
        for (auto it = m_rank_sampler.begin(); it != m_rank_sampler.end(); ++it) {
            result += (*it)->capacity();
        }
        return result;
    }

    void ProfileSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, std::shared_ptr<Comm> comm)
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
            if (m_ctl_msg->is_sample_end()) {  // M_STATUS_SAMPLE_END
                comm->barrier();
                m_ctl_msg->step();
                while (!m_ctl_msg->is_name_begin() &&
                       !m_ctl_msg->is_shutdown()) {
                    geopm_signal_handler_check();
                }
                if (m_ctl_msg->is_name_begin()) {  // M_STATUS_NAME_BEGIN
                    region_names();
                }
            }
        }
        else if (!m_ctl_msg->is_shutdown()) {
            throw Exception("ProfileSampler: invalid application status, expected shutdown status", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    bool ProfileSampler::do_shutdown(void) const
    {
        return m_ctl_msg->is_shutdown();
    }

    bool ProfileSampler::do_report(void) const
    {
        return m_do_report;
    }

    void ProfileSampler::region_names(void)
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
                throw Exception("ProfileSampler::region_names(): Application shutdown while report was being generated", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_rank_sampler.front()->report_name(m_report_name);
        m_rank_sampler.front()->profile_name(m_profile_name);
        // report and profile names may have been inserted as region names
        m_name_set.erase(m_report_name);
        m_name_set.erase(m_profile_name);

        m_do_report = true;

        m_ctl_msg->wait();  // M_STATUS_NAME_END
        m_ctl_msg->step();  // M_STATUS_NAME_END
        m_ctl_msg->wait();  // M_STATUS_SHUTDOWN
    }

    std::set<std::string> ProfileSampler::name_set(void) const
    {
        return m_name_set;
    }

    std::string ProfileSampler::report_name(void) const
    {
        return m_report_name;
    }

    std::string ProfileSampler::profile_name(void) const
    {
        return m_profile_name;
    }

    std::shared_ptr<IProfileThreadTable> ProfileSampler::tprof_table(void) const
    {
        return m_tprof_table;
    }

    void ProfileSampler::abort(void)
    {
        m_ctl_msg->abort();
    }

    ProfileRankSampler::ProfileRankSampler(const std::string shm_key, size_t table_size)
        : m_table_shmem(nullptr)
        , m_table(nullptr)
        , m_region_entry(GEOPM_INVALID_PROF_MSG)
        , m_is_name_finished(false)
    {
        std::string key_path("/dev/shm/" + shm_key);
        (void)unlink(key_path.c_str());
        errno = 0; // Ignore errors from the unlink call.
        m_table_shmem = geopm::make_unique<SharedMemory>(shm_key, table_size);
        m_table = geopm::make_unique<ProfileTable>(m_table_shmem->size(), m_table_shmem->pointer());
    }

    size_t ProfileRankSampler::capacity(void) const
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

    void ProfileRankSampler::report_name(std::string &report_str) const
    {
        report_str = m_report_name;
    }

    void ProfileRankSampler::profile_name(std::string &prof_str) const
    {
        prof_str = m_prof_name;
    }
}
