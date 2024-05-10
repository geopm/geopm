/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_FIELD_H_INCLUDE
#define GEOPM_FIELD_H_INCLUDE
#include <stdint.h>
#include <string.h>

#include "geopm_public.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Convert a signal that is implicitly a 64-bit field
 *        especially useful for converting region IDs.
 * @param [in] signal value returned by PlatformIO::sample() or
 *        PlatformIO::read_signal() for a signal with a name that
 *        ends with the '#' character.
 */
static inline uint64_t geopm_signal_to_field(double signal)
{
    uint64_t result;
    static_assert(sizeof result == sizeof signal,
                  "geopm_signal_to_field requires that signals and fields are the same size");
    memcpy(&result, &signal, sizeof(result));
    return result;
}

/*!
 * @brief Convert a 64-bit field into a double representation
 *        appropriate for a signal returned by an IOGroup.
 * @param [in] field Arbitrary 64-bit field to be stored in a
 *        double precision value.
 */
static inline double geopm_field_to_signal(uint64_t field)
{
    double result;
    static_assert(sizeof result == sizeof field,
                  "geopm_field_to_signal requires that signals and fields are the same size");
    memcpy(&result, &field, sizeof(result));
    return result;
}

#ifdef __cplusplus
}
#endif
#endif
