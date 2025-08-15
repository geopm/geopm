/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROLLOVERGENERATOR_HPP_INCLUDE
#define ROLLOVERGENERATOR_HPP_INCLUDE

#include <cstdint>

namespace geopm
{
    class RolloverGenerator
    {
        public:
            RolloverGenerator();
            void set_factor(double rollover_factor);
            double update(double value);
        private:
            double m_last_value;
            double m_rollover_factor;
            double m_rollover_total;
    };
}

#endif
