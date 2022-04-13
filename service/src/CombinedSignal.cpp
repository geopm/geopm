/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CombinedSignal.hpp"

#include <cmath>
#include <numeric>
#include <algorithm>

#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "config.h"

namespace geopm
{
    CombinedSignal::CombinedSignal()
        : CombinedSignal(Agg::sum)
    {

    }

    CombinedSignal::CombinedSignal(std::function<double(const std::vector<double> &)> func)
        : m_agg_function(func)
    {

    }

    double CombinedSignal::sample(const std::vector<double> &values)
    {
        return m_agg_function(values);
    }

    DerivativeCombinedSignal::DerivativeCombinedSignal()
        : M_NUM_SAMPLE_HISTORY(8)
        , m_history(M_NUM_SAMPLE_HISTORY)
        , m_derivative_num_fit(0)
    {

    }

    double DerivativeCombinedSignal::sample(const std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        // caller is expected to pass in vector of (time, value).
        if (values.size() != 2) {
            throw Exception("PerRegionDerivativeCombinedSignal::sample(): expected 2 values.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double ins_time = values[0];
        double ins_signal = values[1];
        // insert time and signal
        m_history.insert({ins_time, ins_signal});
        if (m_derivative_num_fit < M_NUM_SAMPLE_HISTORY) {
            ++m_derivative_num_fit;
        }

        // Least squares linear regression to approximate the
        // derivative with noisy data.
        double result = NAN;
        if (m_derivative_num_fit >= 2) {
            size_t buf_size = m_history.size();
            double A = 0.0, B = 0.0, C = 0.0, D = 0.0;
            double E = 1.0 / m_derivative_num_fit;
            double time_0 = m_history.value(buf_size - m_derivative_num_fit).time;
            const double sig_0 = m_history.value(buf_size - m_derivative_num_fit).sample;
            for (size_t buf_off = buf_size - m_derivative_num_fit;
                 buf_off < buf_size; ++buf_off) {
                double tt = m_history.value(buf_off).time;
                double time = tt - time_0;
                double sig = m_history.value(buf_off).sample - sig_0;
                A += time * sig;
                B += time;
                C += sig;
                D += time * time;
            }
            double ssxx = D - B * B * E;
            double ssxy = A - B * C * E;
            result = ssxy / ssxx;
        }
        return result;
    }

    double DifferenceCombinedSignal::sample(const std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != 2) {
            throw Exception("DifferenceCombinedSignal::sample(): expected two values to subtract for temperature.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return values[0] - values[1];
    }
}
