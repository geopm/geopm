/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            /// @param [in] min_hysteresis_base_period Minimum period length in
            ///        number of records that will be considered
            ///        stable in order to begin emitting epoch
            ///        records.  Suggested default is 4.
            ///
            /// @param [in] min_detectable_period Minimum period length in
            ///        number of records that is considered by the algorithm as
            ///        potentially a period.  Suggested default is 3.
            ///
            /// @param [in] stable_period_hysteresis Factor that along
            ///        with the period length that determines when a
            ///        stable period has been detected.  Suggested
            ///        default is 1.
            ///
            /// @param [in] unstable_period_hysteresis Factor that
            ///        along with the period length that determines
            ///        when the period has become unstable.  This
            ///        happens when the period changes from the
            ///        previously detected length.  Suggested default
            ///        is 1.5.
            EditDistEpochRecordFilter(int history_buffer_size,
                                      int min_hysteresis_base_period,
                                      int min_detectable_period,
                                      double stable_period_hysteresis,
                                      double unstable_period_hysteresis);
            EditDistEpochRecordFilter(std::shared_ptr<EditDistPeriodicityDetector> edpd,
                                      int min_hysteresis_base_period,
                                      int min_detectable_period,
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
                                   int &min_hysteresis_base_period,
                                   int &min_detectable_period,
                                   double &stable_period_hysteresis,
                                   double &unstable_period_hysteresis);
        private:
            /// Implements:
            ///  1. The stable period detector state machine,
            ///  2. Returns True if the last record passed to m_edpd
            ///     is considered as an epoch marker. Marked
            ///     records are expected to be spaced (in number of
            ///     records) the length of a period plus/minus erroneous
            ///     records.
            ///
            /// STATE MACHINE OPERATION
            /// -------------------------
            /// PERIOD_DETECTED state means we are observing a stable period of N and as long as we stay in this
            /// state we expect to observe an epoch distance of length N plus/minus erroneously added/removed
            /// calls, which are indicated by the edit distance (score).
            ///
            /// The period stability is measured by comparing the period detected with the most current record vs
            /// detected with the previous. We store the previous in m_last_period.
            ///
            /// The conditions for NO_PERIOD_DETECTED -> PERIOD_DETECTED state transition:
            ///    * Stable period of N detected by String Edit Distance algorithm for
            ///      the last MAX(N, min_hysteresis_base_period) x STABLE_PERIOD_HYSTERESIS records.
            ///    * Only periods >= MIN_DETECTABLE_PERIOD and score greater than the period are considered as
            ///      potentially stable.
            ///
            /// The conditions for PERIOD_DETECTED -> NO_PERIOD_DETECTED state transition:
            ///    * Detected period deviated from N for the last UNSTABLE_PERIOD_HYSTERESIS x N records.
            ///
            /// The conditions for reporting a record as epoch marker:
            ///    * "N plus/minus erroneous records" passed since last reported epoch marker.
            bool epoch_detected();

            /// The String Edit Distance algorithm that finds the
            /// patterns are implemented in this object.
            std::shared_ptr<EditDistPeriodicityDetector> m_edpd;
            // Parameter for the epoch detection algorithm. See
            // EditDistEpochRecordFilter::epoch_detected().
            const int m_min_hysteresis_base_period;
            // Parameter for the epoch detection algorithm. See
            // EditDistEpochRecordFilter::epoch_detected().
            const int m_min_detectable_period;
            // Parameter for the epoch detection algorithm. See
            // EditDistEpochRecordFilter::epoch_detected().
            const double m_stable_period_hysteresis;
            // Parameter for the epoch detection algorithm. See
            // EditDistEpochRecordFilter::epoch_detected().
            const double m_unstable_period_hysteresis;
            // Input to the stable period detector state machine.
            int m_last_period;
            // Input to the stable period detector state machine.
            int m_period_stable;
            // Input to the stable period detector state machine.
            int m_period_unstable;
            // State variable of the stable period detector state machine.
            bool m_is_period_detected;
            // Input to the stable period detector state machine.
            int m_last_epoch;
            // Variable for the filter method.
            int m_epoch_count;
    };
}

#endif
