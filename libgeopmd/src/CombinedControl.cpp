/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CombinedControl.hpp"

namespace geopm
{
    CombinedControl::CombinedControl()
        : CombinedControl(1.0)
    {

    }

    CombinedControl::CombinedControl(double factor)
        : m_factor(factor)
    {

    }

    double CombinedControl::adjust(double setting)
    {
        return m_factor * setting;
    }
}
