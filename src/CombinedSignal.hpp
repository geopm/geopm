/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef COMBINEDSIGNAL_HPP_INCLUDE
#define COMBINEDSIGNAL_HPP_INCLUDE

#include <map>
#include <functional>
#include <vector>

#include "CircularBuffer.hpp"

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
