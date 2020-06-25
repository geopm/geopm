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

#ifndef EDITDISTEPOCHRECORDFILTER_HPP_INCLUDE
#define EDITDISTEPOCHRECORDFILTER_HPP_INCLUDE

#include "RecordFilter.hpp"

namespace geopm
{
    struct record_s;
    class EditDistPeriodicityDetector;

    class EditDistEpochRecordFilter : public RecordFilter
    {
        public:
            /// @brief Default constructor for the filter.
            ///
            /// @param [in] history_buffer_size Number of region entry
            ///        events stored in order to determine an epoch.
            ///
            /// @param [in] min_stable_period Minimum period length in
            ///        number of records that will be considered
            ///        stable in order to begin emitting epoch
            ///        records.  Suggested default is 4.
            ///
            /// @param [in] stable_period_hysteresis Factor that along
            ///        with the period length that detemines when a
            ///        stable period has been detected.  Suggested
            ///        default is 1.
            ///
            /// @param [in] unstable_period_hysteresis Factor that
            ///        along with the period length that detemines
            ///        when the period has become unstable.  This
            ///        happens when the period changes from the
            ///        previously detected length.  Suggested default
            ///        is 1.5.
            EditDistEpochRecordFilter(int history_buffer_size,
                                      int min_stable_period,
                                      double stable_period_hysteresis,
                                      double unstable_period_hysteresis);
            EditDistEpochRecordFilter(std::shared_ptr<EditDistPeriodicityDetector> edpd,
                                      int min_stable_period,
                                      double stable_period_hysteresis,
                                      double unstable_period_hysteresis);
            EditDistEpochRecordFilter(const std::string &name);
            virtual ~EditDistEpochRecordFilter() = default;
            std::vector<record_s> filter(const record_s &record) override;
            /// @brief Static function that will parse the filter
            ///        string for the edit_distance into the constructor
            ///        arguments for a EditDistanceEpochRecordFilter.
            ///        Failure to parse will result in a thrown
            ///        Exception with GEOPM_ERROR_INVALID type.
            static void parse_name(const std::string &name,
                                   int &history_buffer_size,
                                   int &min_stable_period,
                                   double &stable_period_hysteresis,
                                   double &unstable_period_hysteresis);
        private:
            bool epoch_detected();

            std::shared_ptr<EditDistPeriodicityDetector> m_edpd;
            const int m_min_stable_period;
            const double m_stable_period_hysteresis;
            const double m_unstable_period_hysteresis;
            int m_last_period;
            int m_period_stable;
            int m_period_unstable;
            bool m_is_period_detected;
            int m_last_epoch;
            int m_epoch_count;
            int m_record_count;
    };
}

#endif
