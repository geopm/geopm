/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "DerivativeSignal.hpp"

#include <cmath>
#include <unistd.h>

#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    DerivativeSignal::DerivativeSignal(std::shared_ptr<Signal> time_sig,
                                       std::shared_ptr<Signal> y_sig,
                                       int num_sample_history,
                                       double sleep_time)
        : m_time_sig(std::move(time_sig))
        , m_y_sig(std::move(y_sig))
        , M_NUM_SAMPLE_HISTORY(num_sample_history)
        , m_history(M_NUM_SAMPLE_HISTORY)
        , m_derivative_num_fit(0)
        , m_is_batch_ready(false)
        , m_sleep_time(sleep_time)
        , m_last_result(NAN)
    {
        GEOPM_DEBUG_ASSERT(m_time_sig && m_y_sig,
                           "Signal pointers for time_sig and y_sig cannot be null.");
    }

    void DerivativeSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_time_sig->setup_batch();
            m_y_sig->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double DerivativeSignal::compute_next(CircularBuffer<m_sample_s> &history,
                                          int &num_fit,
                                          double time, double signal)
    {
        // insert time and signal
        history.insert({time, signal});
        if (num_fit < history.capacity()) {
            ++num_fit;
        }

        // Least squares linear regression to approximate the
        // derivative with noisy data.
        double result = NAN;
        if (num_fit >= 2) {
            size_t buf_size = history.size();
            double A = 0.0, B = 0.0, C = 0.0, D = 0.0;
            double E = 1.0 / num_fit;
            double time_0 = history.value(buf_size - num_fit).time;
            const double sig_0 = history.value(buf_size - num_fit).sample;
            for (size_t buf_off = buf_size - num_fit;
                 buf_off < buf_size; ++buf_off) {
                double tt = history.value(buf_off).time;
                double dt = tt - time_0;
                double sig = history.value(buf_off).sample - sig_0;
                A += dt * sig;
                B += dt;
                C += sig;
                D += dt * dt;
            }

            double ssxx = D - B * B * E;
            double ssxy = A - B * C * E;
            if (ssxx != 0) {
                result = ssxy / ssxx;
            }
        }
        return result;
    }

    double DerivativeSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double time = m_time_sig->sample();
        size_t history_size = m_history.size();
        // Check if this is the first call ever to sample() (history_size == 0)
        // Or check if this is the first call to sample() since the last call to read_batch() (last element of history buffer does not match the sampled time)
        if (history_size == 0ULL ||
            time != m_history.value(m_history.size() - 1).time) {
            double signal = m_y_sig->sample();
            m_last_result = compute_next(m_history, m_derivative_num_fit, time, signal);
        }
        return m_last_result;
    }

    double DerivativeSignal::read(void) const
    {
        double result = NAN;
        CircularBuffer<m_sample_s> temp_history(M_NUM_SAMPLE_HISTORY);
        int num_fit = 0;
        for (int ii = 0; ii < M_NUM_SAMPLE_HISTORY; ++ii) {
            double signal = m_y_sig->read();
            double time = m_time_sig->read();
            result = compute_next(temp_history, num_fit, time, signal);
            if (ii < M_NUM_SAMPLE_HISTORY - 1) {
                usleep(m_sleep_time * 1e6);
            }
        }
        return result;
    }

}
