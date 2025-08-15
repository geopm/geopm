/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RolloverGenerator.hpp"

namespace geopm
{
    RolloverGenerator::RolloverGenerator()
        : m_last_value(0.0)
        , m_rollover_factor(0.0)
        , m_rollover_total(0.0)
    {

    }

    void RolloverGenerator::set_factor(double rollover_factor)
    {
        m_rollover_factor = rollover_factor;
    }

    double RolloverGenerator::update(double value)
    {
        if (m_last_value > value) {
            m_rollover_total += m_rollover_factor;
        }
        m_last_value = value;
        return value + m_rollover_total;
    }
}
