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
    int geopm_prof_create(const char *name, int sample_reduce, const char *sample_key, struct geopm_prof_c **prof)
    {
        int err = 0;
        try {
            throw geopm::Exception("Profile::geopm_prof_create()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_destroy()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_register()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_enter()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_exit()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_progress()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_outer_sync()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_sample()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_enable()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
            throw geopm::Exception("Profile::geopm_prof_disable()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;

    }

    int geopm_prof_print(struct geopm_prof_c *prof, int depth)
    {
        return geopm_prof_fprint(prof, depth, stdout);
    }

    int geopm_prof_fprint(struct geopm_prof_c *prof, int depth, FILE *fid)
    {
        int err = 0;
        try {
            throw geopm::Exception("Profile::geopm_prof_fprint()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
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
    Profile::Profile(const std::string name, const std::string sample_key, int sample_reduce)
    : m_name(name)
    , m_sample_key(sample_key)
    , m_sample_reduce(sample_reduce)
    {
        throw geopm::Exception("class Profile", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    Profile::~Profile()
    {

    }
}
