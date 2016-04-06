/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <math.h>
#include <cfloat>
#include <stdexcept>
#include <algorithm>

#include "Exception.hpp"
#include "Observation.hpp"

namespace geopm
{
    void Observation::allocate_buffer(int &index, int window_size)
    {
        CircularBuffer<double> b(window_size);
        index = m_data.size();
        m_data.push_back(b);
    }

    void Observation::insert(int buffer_index, double value)
    {
        if ((unsigned int)buffer_index < m_data.size()) {
            m_data[buffer_index].insert(value);
        }
        else {
            throw Exception("Observation: unknown data type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    double Observation::mean(int buffer_index) const
    {
        double result = 0.0;
        double sum = 0.0;
        size_t len = 0;

        if ((unsigned int)buffer_index >= m_data.size()) {
            throw Exception("Observation: unknown data type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (int i = 0; i < m_data[buffer_index].size(); i++) {
            sum += m_data[buffer_index].value(i);
            len++;
        }
        if (len) {
            result = sum / len;
        }
        else {
            throw Exception("Observation: data vector of zero length", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return result;
    }

    double Observation::median(int buffer_index) const
    {
        double result = 0.0;
        size_t len = 0;
        std::vector<double> sorted;

        if ((unsigned int)buffer_index >= m_data.size()) {
            throw Exception("Observation: unknown data type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        len = m_data[buffer_index].size();
        if (len) {
            for (int i = 0; i < m_data[buffer_index].size(); i++) {
                sorted.push_back(m_data[buffer_index].value(i));
            }
            std::sort(sorted.begin(), sorted.end());
            result = sorted[(len - 1)/ 2];
        }
        else {
            throw Exception("Observation: data vector of zero length", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    double Observation::stddev(int buffer_index) const
    {
        double result = 0.0;
        double sum = 0.0;
        double mm, tmp;
        size_t len = 0;

        /* mean will throw if index doesn't exist */
        mm = mean(buffer_index);
        len = m_data[buffer_index].size();
        if (len > 1) {
            for (int i = 0; i < m_data[buffer_index].size(); i++) {
                tmp = m_data[buffer_index].value(i) - mm;
                tmp *= tmp;
                sum += tmp;
            }
            result = sqrt(sum / (len - 1));
        }
        else {
            throw Exception("Observation: data vector of zero length", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    double Observation::max(int buffer_index) const
    {
        double result = -DBL_MAX;
        int len = 0;

        if ((unsigned int)buffer_index >= m_data.size()) {
            throw Exception("Observation: unknown data type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        len = m_data[buffer_index].size();
        if (len) {
            for (int i = 0; i < m_data[buffer_index].size(); i++) {
                if (m_data[buffer_index].value(i) > result) {
                    result = m_data[buffer_index].value(i);
                }
            }
        }
        else {
            throw Exception("Observation: data vector of zero length", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    double Observation::min(int buffer_index) const
    {
        double result = DBL_MAX;
        int len = 0;

        if ((unsigned int)buffer_index >= m_data.size()) {
            throw Exception("Observation: unknown data type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        len = m_data[buffer_index].size();
        if (len) {
            for (int i = 0; i < m_data[buffer_index].size(); i++) {
                if (m_data[buffer_index].value(i) < result) {
                    result = m_data[buffer_index].value(i);
                }
            }
        }
        else {
            throw Exception("Observation: data vector of zero length", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    double Observation::integrate_time(int buffer_index) const
    {
        double result = 0.0;
        double time, delta;
        int len = m_data[buffer_index].size();

        if (m_data[0].size() != len) {
            throw Exception("Observation: cannot integrate buffer over time, length doesn't match timestamp buffer", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        if (len) {
            time = m_data[0].value(0);
            for (int i = 1; i < len; ++i) {
                delta = m_data[0].value(i) - time;
                result += delta * (m_data[buffer_index].value(i-1) + m_data[buffer_index].value(i))/2;
                time = m_data[0].value(i);
            }
        }
        return result;
    }
}
