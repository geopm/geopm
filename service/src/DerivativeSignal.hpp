/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DERIVATIVESIGNAL_HPP_INCLUDE
#define DERIVATIVESIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"
#include "geopm/CircularBuffer.hpp"

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
            double m_last_result;
    };
}

#endif
