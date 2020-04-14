/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef DERIVATIVESIGNAL_HPP_INCLUDE
#define DERIVATIVESIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"
#include "CircularBuffer.hpp"

namespace geopm
{
    class DerivativeSignal : public Signal
    {
        public:
            DerivativeSignal(std::shared_ptr<Signal> time_sig,
                             std::shared_ptr<Signal> y_sig,
                             int read_loops, double sleep_time);
            DerivativeSignal(const DerivativeSignal &other) = delete;
            virtual ~DerivativeSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            struct m_sample_s {
                double time;
                double sample;
            };

            /// Update the history buffer and compute the new derivative.
            /// The read() and sample() methods have separate history.
            static double compute_next(CircularBuffer<m_sample_s> &history,
                                       int &num_fit,
                                       double time, double signal);

            std::shared_ptr<Signal> m_time_sig;
            std::shared_ptr<Signal> m_y_sig;

            const int M_NUM_SAMPLE_HISTORY;
            CircularBuffer<m_sample_s> m_history;
            int m_derivative_num_fit;
            bool m_is_batch_ready;
            double m_sleep_time;
    };
}

#endif
