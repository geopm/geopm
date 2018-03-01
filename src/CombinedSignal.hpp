/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "CircularBuffer.hpp"


namespace geopm
{
    class CombinedSignal
    {
        public:
            CombinedSignal() = default;
            virtual ~CombinedSignal() = default;
            virtual double sample(const std::vector<double> &values);
    };

    class PerRegionDerivativeCombinedSignal : public CombinedSignal
    {
        public:
            PerRegionDerivativeCombinedSignal() = default;
            virtual ~PerRegionDerivativeCombinedSignal() = default;
            double sample(const std::vector<double> &values) override;
        protected:
            struct m_sample_s
            {
                double time;
                double sample;
            };
            // map from region ID to time+energy history for that region
            std::map<double, CircularBuffer<m_sample_s> > m_history;
            std::map<double, double> m_derivative_last;
            std::map<double, int> m_derivative_num_fit;
            const int M_NUM_SAMPLE_HISTORY = 8;
    };
}

#endif
