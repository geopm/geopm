/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_TIME_H_INCLUDE
#define GEOPM_TIME_H_INCLUDE
#include <math.h>

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
static inline void geopm_time_add(const struct geopm_time_s *begin, double elapsed, struct geopm_time_s *end);

#ifdef __linux__
#include <time.h>

/*!
 * @brief structure to abstract the difference between a timespec on linux or a timeval on OSX.
 */
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

static inline void geopm_time_add(const struct geopm_time_s *begin, double elapsed, struct geopm_time_s *end)
{
    *end = *begin;
    end->t.tv_sec += elapsed;
    elapsed -= floor(elapsed);
    end->t.tv_nsec += 1E9 * elapsed;
    if (end->t.tv_nsec >= 1000000000) {
        end->t.tv_nsec -= 1000000000;
        ++(end->t.tv_sec);
    }
}

#else
#include <sys/time.h>

/*!
 * @brief structure to abstract the difference between a timespec on linux or a timeval on OSX.
 */
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

static inline void geopm_time_add(const struct geopm_time_s *begin, double elapsed, struct geopm_time_s *end)
{
    *end = *begin;
    end->t.tv_sec += elapsed;
    elapsed -= floor(elapsed);
    end->t.tv_usec += 1E6 * elapsed;
}

#endif

#ifdef __cplusplus
}
#endif
#endif
