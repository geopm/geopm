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

#ifndef EDITDISTPERIODICITYDETECTOR_HPP_INCLUDE
#define EDITDISTPERIODICITYDETECTOR_HPP_INCLUDE

#include <cstdint>
#include <vector>

#include "CircularBuffer.hpp"

namespace geopm
{
    struct record_s;

    class EditDistPeriodicityDetector
    {
        public:
            EditDistPeriodicityDetector(int history_buffer_size);
            virtual ~EditDistPeriodicityDetector() = default;
            /// @brief Update detector with a new record from the application.
            void update(const record_s &record);
            /// @brief Return the best estimate of the period length
            ///        in number of records, based on the data
            ///        inserted through update().  Until a stable
            ///        period is determined, returns -1.
            int get_period(void) const;
            /// @brief Return the metric that will be maximized to
            ///        detemine the period.  Until a stable period is
            ///        determined, returns -1.
            int get_score(void) const;
            /// @brief Return the number of records that this object has
            ///        received so far via update().
            int num_records(void) const;
        private:
            void calc_period();
            unsigned int Dget(int ii, int jj, int mm);
            void Dset(int ii, int jj, int mm, unsigned int val);
            uint64_t get_history_value(int index) const;
            int find_smallest_repeating_pattern(int index) const;

            CircularBuffer<uint64_t> m_history_buffer;
            int m_history_buffer_size;
            int m_period;
            int m_score;
            int m_record_count;

            unsigned int m_myinf;
            unsigned int *m_DP;
    };
}

#endif
