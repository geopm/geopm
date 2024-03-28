/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "TimeSignal.hpp"

#include "geopm_time.h"
#include "geopm/Exception.hpp"

namespace geopm
{
    TimeSignal::TimeSignal(std::shared_ptr<geopm_time_s> time_zero,
                           std::shared_ptr<double> time_batch)
        : m_time_zero(std::move(time_zero))
        , m_time_batch(std::move(time_batch))
        , m_is_batch_ready(false)
    {

    }

    void TimeSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_is_batch_ready = true;
        }
    }

    double TimeSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return *m_time_batch;
    }

    double TimeSignal::read(void) const
    {
        return geopm_time_since(m_time_zero.get());
    }
}
