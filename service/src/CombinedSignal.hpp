/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

    /// @brief Used by PlatformIO for CombinedSignals based on a
    ///        derivative of another signal over time.
    class DerivativeCombinedSignal : public CombinedSignal
    {
        public:
            DerivativeCombinedSignal();
            virtual ~DerivativeCombinedSignal() = default;
            double sample(const std::vector<double> &values) override;
        private:
            struct m_sample_s {
                double time;
                double sample;
            };
            const int M_NUM_SAMPLE_HISTORY;
            // time + energy history
            CircularBuffer<m_sample_s> m_history;
            int m_derivative_num_fit;
    };

    /// @brief Used by PlatformIO for CombinedSignals based on a
    ///        difference between two signals.
    class DifferenceCombinedSignal : public CombinedSignal
    {
        public:
            DifferenceCombinedSignal() = default;
            virtual ~DifferenceCombinedSignal() = default;
            double sample(const std::vector<double> &values) override;
    };
}

#endif
