/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_TIME_H_INCLUDE
#define GEOPM_TIME_H_INCLUDE

#include <errno.h>
#include <math.h>
#include <string.h>

#include "geopm_public.h"

#ifndef __cplusplus
#include <stdbool.h>
#else
#include <string>
extern "C"
{
#endif

/// @brief String allocation size sufficient for results of geopm_time function calls
#define GEOPM_TIME_STRING_MAX 255ULL

struct geopm_time_s;

static inline int geopm_time_string(int buf_size, char *buf);
static inline int geopm_time(struct geopm_time_s *time);
static inline double geopm_time_diff(const struct geopm_time_s *begin, const struct geopm_time_s *end);
static inline bool geopm_time_comp(const struct geopm_time_s *aa, const struct geopm_time_s *bb);
static inline void geopm_time_add(const struct geopm_time_s *begin, double elapsed, struct geopm_time_s *end);
static inline double geopm_time_since(const struct geopm_time_s *begin);

int GEOPM_PUBLIC
    geopm_time_zero(struct geopm_time_s *zero_time);

#include <time.h>

/*!
 * @brief structure to abstract the timespec on linux from other
 *        representations of time.
 */
struct GEOPM_PUBLIC geopm_time_s {
    struct timespec t;
};

static inline int geopm_time(struct geopm_time_s *time)
{
    return clock_gettime(CLOCK_MONOTONIC_RAW, &(time->t));
}

static inline int geopm_time_real(struct geopm_time_s *time)
{
    return clock_gettime(CLOCK_REALTIME, &(time->t));
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

static inline int geopm_time_to_string(const struct geopm_time_s *time, int buf_size, char *buf)
{
    int err = 0;
    struct tm tm;
    struct geopm_time_s ref_time_real;
    struct geopm_time_s ref_time_mono;
    clock_gettime(CLOCK_REALTIME, &(ref_time_real.t));
    clock_gettime(CLOCK_MONOTONIC_RAW, &(ref_time_mono.t));
    time_t sec_since_1970 = geopm_time_diff(&ref_time_mono, &ref_time_real) + time->t.tv_sec;
    localtime_r(&sec_since_1970, &tm);
    size_t num_byte = strftime(buf, buf_size, "%a %b %d %H:%M:%S %Y", &tm);
    if (!num_byte) {
        err = EINVAL;
    }
    return err;
}

int GEOPM_PUBLIC
    geopm_time_real_to_iso_string(const struct geopm_time_s *time, int buf_size, char *buf);

static inline int geopm_time_string(int buf_size, char *buf)
{
    struct geopm_time_s time;
    int err = geopm_time_real(&time);
    if (!err) {
        err = geopm_time_real_to_iso_string(&time, buf_size, buf);
    }
    return err;
}

static inline double geopm_time_since(const struct geopm_time_s *begin)
{
    struct geopm_time_s curr_time;
    geopm_time(&curr_time);
    return geopm_time_diff(begin, &curr_time);
}

#ifdef __cplusplus
}
namespace geopm
{
    struct geopm_time_s GEOPM_PUBLIC
        time_zero(void);
    struct geopm_time_s GEOPM_PUBLIC
        time_curr(void);
    struct geopm_time_s GEOPM_PUBLIC
        time_curr_real(void);
    std::string GEOPM_PUBLIC
        time_curr_string(void);
    void GEOPM_PUBLIC
        time_zero_reset(const geopm_time_s &zero);
}
#endif
#endif
