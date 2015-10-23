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
#include "Profile.hpp"
#include "Exception.hpp"

extern "C"
{
    int geopm_prof_create(const char *name, int sample_reduce, struct geopm_sample_shmem_s *sample, struct geopm_prof_c **prof)
    {
        int err = 0;
        try {
            *prof = (struct geopm_prof_c *)(new geopm::Profile(std::string(name), sample_reduce, sample));
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

    int geopm_prof_register(struct geopm_prof_c *prof, const char *region_name, long policy_hint, int *region_id)
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

    int geopm_prof_enter(struct geopm_prof_c *prof, int region_id)
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

    int geopm_prof_exit(struct geopm_prof_c *prof,
                        int region_id)
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

    int geopm_prof_progress(struct geopm_prof_c *prof, int region_id, double fraction)
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

    int geopm_prof_sample(struct geopm_prof_c *prof)
    {
        int err = 0;
        try {
            geopm::Profile *prof_obj = (geopm::Profile *)prof;
            if (prof_obj == NULL) {
                throw geopm::Exception(GEOPM_ERROR_PROF_NULL, __FILE__, __LINE__);
            }
            prof_obj->sample();
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
    Profile::Profile(const std::string name, int sample_reduce, struct geopm_sample_shmem_s *sample_shmem)
    : m_name(name)
    , m_sample_reduce(sample_reduce)
    , m_sample_shmem(sample_shmem)
    {
        throw geopm::Exception("class Profile", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    Profile::~Profile()
    {

    }

    int Profile::region(const std::string region_name, long policy_hint)
    {
        throw geopm::Exception("Profile::register()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return -1;
    }

    void Profile::enter(int region_id)
    {
        throw geopm::Exception("Profile::enter()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::exit(int region_id)
    {
        throw geopm::Exception("Profile::exit()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::progress(int region_id, double fraction)
    {
        throw geopm::Exception("Profile::progress()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::outer_sync(void)
    {
        throw geopm::Exception("Profile::outer_sync()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void Profile::sample(void)
    {
        throw geopm::Exception("Profile::sample()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
