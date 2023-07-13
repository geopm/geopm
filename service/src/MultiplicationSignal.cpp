/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "MultiplicationSignal.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    MultiplicationSignal::MultiplicationSignal(std::shared_ptr<Signal> multiplier,
                                               double multiplicand)
        : m_multiplier(std::move(multiplier))
        , m_multiplicand(multiplicand)
        , m_is_batch_ready(false)
    {
        GEOPM_DEBUG_ASSERT(m_multiplicand && m_multiplier,
                           "Signal pointers for multiplier and multiplicand cannot be null.");
    }

    void MultiplicationSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_multiplier->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double MultiplicationSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return m_multiplier->sample() * m_multiplicand;
    }

    double MultiplicationSignal::read(void) const
    {
        return m_multiplier->read() * m_multiplicand;
    }
}
