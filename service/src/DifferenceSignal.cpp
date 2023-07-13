/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "DifferenceSignal.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    DifferenceSignal::DifferenceSignal(std::shared_ptr<Signal> minuend,
                                       std::shared_ptr<Signal> subtrahend)
        : m_minuend(std::move(minuend))
        , m_subtrahend(std::move(subtrahend))
        , m_is_batch_ready(false)
    {
        GEOPM_DEBUG_ASSERT(m_minuend && m_subtrahend,
                           "Signal pointers for minuend and subtrahend cannot be null.");
    }

    void DifferenceSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_minuend->setup_batch();
            m_subtrahend->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double DifferenceSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_minuend->sample() - m_subtrahend->sample();
    }

    double DifferenceSignal::read(void) const
    {
        return m_minuend->read() - m_subtrahend->read();
    }
}
