/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "DivisionSignal.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

#include <limits>

namespace geopm
{
    DivisionSignal::DivisionSignal(std::shared_ptr<Signal> numerator,
                                   std::shared_ptr<Signal> denominator)
        : m_numerator(numerator)
        , m_denominator(denominator)
        , m_is_batch_ready(false)
    {
        GEOPM_DEBUG_ASSERT(m_numerator && m_denominator,
                           "Signal pointers for numerator and denominator cannot be null.");
    }

    void DivisionSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_numerator->setup_batch();
            m_denominator->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double DivisionSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        double result = std::numeric_limits<double>::quiet_NaN();
        double numer = m_numerator->sample();
        double denom = m_denominator->sample();

        if (denom != 0) {
            result = numer / denom;
        }
        return result;
    }

    double DivisionSignal::read(void) const
    {
        double result = std::numeric_limits<double>::quiet_NaN();
        double numer = m_numerator->read();
        double denom = m_denominator->read();

        if (denom != 0) {
            result = numer / denom;
        }
        return result;
    }
}
