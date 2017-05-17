/*
 * Copyright (c) 2016, Intel Corporation
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

#ifndef SAMPLESCHEDULER_HPP_INCLUDE
#define SAMPLESCHEDULER_HPP_INCLUDE
#endif

#include "geopm_time.h"

namespace geopm
{
    /// @brief SampleSchecduler class encapsulates functionality to schedule and
    /// regulate the frequency of application profile samples.
    class SampleSchedulerBase
    {
        public:
            SampleSchedulerBase() {}
            virtual ~SampleSchedulerBase() {}
            virtual bool do_sample(void) = 0;
            virtual void record_exit(void) = 0;
            virtual void clear(void) = 0;
    };

    class SampleScheduler : public SampleSchedulerBase
    {
        public:
            SampleScheduler(double overhead_frac);
            virtual ~SampleScheduler();
            bool do_sample(void);
            void record_exit(void);
            void clear(void);
        protected:
            enum m_status_e {
                M_STATUS_CLEAR,
                M_STATUS_ENTERED,
                M_STATUS_READY,
            };
            double m_overhead_frac;
            int m_status;
            struct geopm_time_s m_entry_time;
            double m_sample_time;
            double m_work_time;
            size_t m_sample_stride;
            size_t m_sample_count;
    };
}
