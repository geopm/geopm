/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EDITDISTPERIODICITYDETECTOR_HPP_INCLUDE
#define EDITDISTPERIODICITYDETECTOR_HPP_INCLUDE

#include <cstdint>
#include <vector>

#include "geopm/CircularBuffer.hpp"

namespace geopm
{
    struct record_s;

    class EditDistPeriodicityDetector
    {
        public:
            /// @brief Default constructor for String Edit Distance based
            ///        periodicity detector used in EditDistEpochRecordFilter.
            ///
            /// @param [in] history_buffer_size Number of region entry
            ///        events stored in order to determine an epoch.
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
            size_t Didx(int ii, int jj, int mm) const;
            uint32_t Dget(int ii, int jj, int mm) const;
            void Dset(int ii, int jj, int mm, uint32_t val);
            uint64_t get_history_value(int index) const;
            int find_smallest_repeating_pattern(int index) const;

            CircularBuffer<uint64_t> m_history_buffer;
            const int m_history_buffer_size;
            int m_period;
            int m_score;
            int m_record_count;
            std::vector<uint32_t> m_DP;
    };
}

#endif
