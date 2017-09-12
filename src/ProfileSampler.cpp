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

#include <algorithm>
#include <sstream>

#include "geopm_time.h"
#include "geopm_signal_handler.h"
#include "Profile.hpp"
#include "ProfileSampler.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "geopm_env.h"
#include "geopm_sched.h"
#include "config.h"
#include "Comm.hpp"

const struct geopm_prof_message_s GEOPM_INVALID_PROF_MSG = {-1, 0, {{0, 0}}, -1.0};

static bool geopm_prof_compare(const std::pair<uint64_t, struct geopm_prof_message_s> &aa, const std::pair<uint64_t, struct geopm_prof_message_s> &bb)
{
    return geopm_time_comp(&(aa.second.timestamp), &(bb.second.timestamp));
}

namespace geopm
{
    ProfileSampler::ProfileSampler(size_t table_size)
        : m_table_size(table_size)
        , m_do_report(false)
        , m_tprof_shmem(NULL)
        , m_tprof_table(NULL)
    {
        std::string sample_key(geopm_env_shmkey());
        sample_key += "-sample";
        m_ctl_shmem = new SharedMemory(sample_key, table_size);
        m_ctl_msg = new ControlMessage((struct geopm_ctl_message_s *)m_ctl_shmem->pointer(), true, true);

        std::string tprof_key(geopm_env_shmkey());
        tprof_key += "-tprof";
        size_t tprof_size = 64 * geopm_sched_num_cpu();
        m_tprof_shmem = new SharedMemory(tprof_key, tprof_size);
        m_tprof_table = new ProfileThreadTable(tprof_size, m_tprof_shmem->pointer());
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
        if (rank_per_node == 0) {
            m_ctl_msg->abort();
            throw Exception("ProfileSampler::initialize(): Application ranks were not list as running on any CPUs.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
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

    void ProfileSampler::sample(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > &content, size_t &length, IComm *comm)
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
