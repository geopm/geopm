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

#include "Exception.hpp"
#include "SampleScheduler.hpp"
#include "config.h"

namespace geopm
{
    SampleScheduler::SampleScheduler(double overhead_frac)
        : m_overhead_frac(overhead_frac)
        , m_status(M_STATUS_CLEAR)
    {

    }

    bool SampleScheduler::do_sample(void)
    {
        bool result = true;
        switch (m_status) {
            case M_STATUS_CLEAR:
                geopm_time(&m_entry_time);
                m_sample_time = -1.0;
                m_status = M_STATUS_ENTERED;
                break;
            case M_STATUS_ENTERED:
                if (m_sample_time == -1.0) {
                    throw Exception("SampleScheduler::do_sample(): do_sample() called twice without call to record_exit()", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
                m_work_time = geopm_time_since(&m_entry_time);
                m_sample_stride = (size_t)(m_sample_time/(m_overhead_frac * m_work_time)) + 1;
                m_sample_count = 0;
                m_status = M_STATUS_READY;
                break;
            case M_STATUS_READY:
                ++m_sample_count;
                if (m_sample_count == m_sample_stride) {
                    m_sample_count = 0;
                }
                else {
                    result = false;
                }
                break;
            default:
                throw Exception("SampleScheduler::do_sample(): Status has invalid value", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                break;
        }
        return result;
    }
    void SampleScheduler::record_exit(void)
    {
        switch (m_status) {
            case M_STATUS_CLEAR:
                throw Exception("SampleScheduler::record_exit(): record_exit() called without prior call to do_sample()", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                break;
            case M_STATUS_ENTERED:
                m_sample_time = geopm_time_since(&m_entry_time);
                break;
            case M_STATUS_READY:
                break;
            default:
                throw Exception("SampleScheduler::do_sample(): Status has invalid value", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                break;
        }
    }

    void SampleScheduler::clear(void)
    {
        m_status = M_STATUS_CLEAR;
    }
}
