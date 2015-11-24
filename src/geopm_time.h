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
#ifndef GEOPM_TIME_H_INCLUDE
#define GEOPM_TIME_H_INCLUDE
#ifndef __cplusplus
#include <stdbool.h>
#else
extern "C"
{
#endif

struct geopm_time_s;

static inline int geopm_time(struct geopm_time_s *time);
static inline double geopm_time_diff(const struct geopm_time_s *begin, const struct geopm_time_s *end);
static inline bool geopm_time_comp(const struct geopm_time_s *aa, const struct geopm_time_s *bb);

//#ifdef __linux__ FIXME not sure why clock_gettime() is no longer defined
#if 0
#include <time.h>

struct geopm_time_s {
    struct timespec t;
};

static inline int geopm_time(struct geopm_time_s *time)
{
    return clock_gettime(CLOCK_MONOTONIC_RAW, &(time->t));
}

static inline double geopm_time_diff(const struct geopm_time_s *begin, const struct geopm_time_s *end)
{
    return (end->t.tv_sec - begin->t.tv_sec) +
           (end->t.tv_nsec - begin->t.tv_nsec) * 1E-9;
}

static inline bool geopm_time_comp(const struct geopm_time_s *aa, const struct geopm_time_s *bb)
{
    bool result = aa->t.tv_sec < bb->t.tv_sec;
    if (!result && aa->t.tv_sec == bb->t.tv_sec) {
        result = aa->t.tv_nsec < bb->t.tv_nsec;
    }
    return result;
}

#else
#include <sys/time.h>

struct geopm_time_s {
    struct timeval t;
};

static inline int geopm_time(struct geopm_time_s *time)
{
    return gettimeofday((struct timeval *)time, NULL);
}

static inline double geopm_time_diff(const struct geopm_time_s *begin, const struct geopm_time_s *end)
{
    return (end->t.tv_sec - begin->t.tv_sec) +
           (end->t.tv_usec - begin->t.tv_usec) * 1E-6;
}

static inline bool geopm_time_comp(const struct geopm_time_s *aa, const struct geopm_time_s *bb)
{
    bool result = aa->t.tv_sec < bb->t.tv_sec;
    if (!result && aa->t.tv_sec == bb->t.tv_sec) {
        result = aa->t.tv_usec < bb->t.tv_usec;
    }
    return result;
}

#endif

#ifdef __cplusplus
}
#endif
#endif
