/*
 * Copyright (c) 2015, Intel Corporation
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

#include <float.h>

#include "geopm.h"
#include "geopm_policy_message.h"
#include "geopm_time.h"
#include "Profile.hpp"
#include "Exception.hpp"
#include "LockingHashTable.hpp"


extern "C"
{
    int geopm_prof_create(const char *name, int sample_reduce, size_t table_size, const char *shm_key, struct geopm_prof_c **prof)
    {
        int err = 0;
        try {
            if (shm_key) {
                *prof = (struct geopm_prof_c *)(new geopm::Profile(std::string(name), sample_reduce, table_size, std::string(shm_key)));
            }
            else {
                *prof = (struct geopm_prof_c *)(new geopm::Profile(std::string(name), sample_reduce, table_size));
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_destroy(struct geopm_prof_c *prof)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            delete prof_obj;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_region(struct geopm_prof_c *prof, const char *region_name, long policy_hint, uint64_t *region_id)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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

    int geopm_prof_sample(struct geopm_prof_c *prof, uint64_t region_id)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->sample(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_enable(struct geopm_prof_c *prof, const char *feature_name)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->enable(std::string(feature_name));
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
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
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

    int geopm_prof_print(struct geopm_prof_c *prof, FILE *fid, int depth)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->print(fid, depth);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    double geopm_progress_threaded_min(int num_thread,
                                       size_t stride,
                                       const uint32_t *progress,
                                       const double *norm)
    {
        double progress_min = DBL_MAX;
        double progress_tmp;
        int j;

        for (j = 0; j < num_thread; ++j) {
            progress_tmp = progress[j * stride] * norm[j];
            progress_min =  progress_tmp < progress_min ?
                            progress_tmp : progress_min;
        }
        return progress_min;
    }

    int geopm_omp_sched_static_norm(int num_iter, int chunk_size, int num_thread, double *norm)
    {
        int remain = num_iter;
        int i = 0;

        /* inefficient but robust way of calculating the norm based on
           OpenMP documentation. */
        memset(norm, 0, sizeof(double) * num_thread);
        while (remain) {
            if (remain > chunk_size) {
                norm[i] += chunk_size;
                remain -= chunk_size;
            }
            else {
                norm[i] += remain;
                remain = 0;
            }
            i++;
            if (i == num_thread) {
                i = 0;
            }
        }
        for (i = 0; i < num_thread; ++i) {
            if (norm[i] != 0.0) {
                norm[i] = 1.0 / norm[i];
            }
        }
        return 0;
    }
}

namespace geopm
{
    Profile::Profile(const std::string name, int sample_reduce, size_t table_size, const std::string shm_key)
        : m_name(name)
        , m_sample_reduce(sample_reduce)
        , m_curr_region_id(0)
        , m_enter_time({{0, 0}})
        , m_num_enter(0)
        , m_num_progress(0)
        , m_progress(0.0)
    {
        if (shm_key.length()) {
            m_shmem = new SharedMemoryUser(shm_key, table_size);
            m_table_buffer = m_shmem->pointer();
            m_table = new LockingHashTable<struct geopm_sample_message_s>(table_size, m_table_buffer);
        }
        else {
            m_shmem = NULL;
            m_table_buffer = malloc(table_size);
            if (!m_table_buffer) {
                throw Exception("Profile: m_table_buffer", ENOMEM, __FILE__, __LINE__);
            }
            m_table = new LockingHashTable<struct geopm_sample_message_s>(table_size, m_table_buffer);
        }
    }

    Profile::Profile(const std::string name, int sample_reduce, size_t table_size)
        : Profile(name, sample_reduce, table_size, "")
    {

    }

    Profile::~Profile()
    {
        delete m_table;
        if (m_shmem) {
            delete m_shmem;
        }
        else {
            free(m_table_buffer);
        }
    }

    uint64_t Profile::region(const std::string region_name, long policy_hint)
    {
        return m_table->key(region_name);
    }

    void Profile::enter(uint64_t region_id)
    {
        if (!m_curr_region_id) {
            m_curr_region_id = region_id;
            m_num_enter = 0;
            (void) geopm_time(&m_enter_time);
        }
        if (m_curr_region_id == region_id) {
            ++m_num_enter;
        }
    }

    void Profile::exit(uint64_t region_id)
    {
        if (m_curr_region_id == region_id) {
            --m_num_enter;
        }
        if (!m_num_enter) {
            struct geopm_sample_message_s sample = m_table->find(region_id);
            sample.phase_id = region_id;
            sample.progress = 1.0;
            struct geopm_time_s exit_time;
            (void) geopm_time(&exit_time);
            sample.runtime = geopm_time_diff(&m_enter_time, &exit_time);
            m_table->insert(region_id, sample);
            m_curr_region_id = 0;
        }
    }

    void Profile::progress(uint64_t region_id, double fraction)
    {
        if (m_num_enter == 1 && m_curr_region_id == region_id) {
            m_progress = fraction;
            ++m_num_progress;
            if (m_num_progress == GEOPM_CONST_PROF_SAMPLE_PERIOD) {
                sample(region_id);
                m_num_progress = 0;
            }
        }
    }

    void Profile::outer_sync(void)
    {
        throw geopm::Exception("Profile::outer_sync()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::sample(uint64_t region_id)
    {
        if (region_id == m_curr_region_id) {
            struct geopm_sample_message_s sample;
            struct geopm_time_s curr_time;
            sample.phase_id = region_id;
            (void) geopm_time(&curr_time);
            sample.runtime = geopm_time_diff(&m_enter_time, &curr_time);
            sample.progress = m_progress;
            sample.energy = 0.0;     // FIXME: need to add a platform to Profile and
            sample.frequency = 0.0;  // update energy and frequency here.
            m_table->insert(region_id, sample);
        }
    }

    void Profile::enable(const std::string feature_name)
    {
        throw geopm::Exception("Profile::enable()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::disable(const std::string feature_name)
    {
        throw geopm::Exception("Profile::disable()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::print(FILE *fid, int depth) const
    {
        throw geopm::Exception("Profile::print()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }
}
