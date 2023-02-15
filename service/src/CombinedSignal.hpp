/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COMBINEDSIGNAL_HPP_INCLUDE
#define COMBINEDSIGNAL_HPP_INCLUDE

#include <map>
#include <functional>
#include <vector>

#include "geopm/CircularBuffer.hpp"

namespace geopm
{
    /// @brief Used by PlatformIO to define a signal as a function of
    ///        other signals.
    class CombinedSignal
    {
        public:
            CombinedSignal();
            CombinedSignal(std::function<double(const std::vector<double> &)>);
            virtual ~CombinedSignal() = default;
            /// @brief Sample all required signals and aggregate
            ///        values to produce the combined signal.
            virtual double sample(const std::vector<double> &values);
            std::function<double(const std::vector<double> &)> m_agg_function;
    };
}

#endif
