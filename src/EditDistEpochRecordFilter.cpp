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

#include "config.h"

#include "EditDistEpochRecordFilter.hpp"
#include "EditDistPeriodicityDetector.hpp"
#include "Exception.hpp"
#include "record.hpp"

// In order to switch state to PERIOD_DETECTED from WAITING
// we require that the period is stable for a certain number of records:
//       period_length x STABLE_PERIOD_HYSTERESIS
#define STABLE_PERIOD_HYSTERESIS 1
// The stable period hysteresis may be adaquate for normal size periods
// but may allow very small period lengths to pass through at the beginning.
// For example:
//    A B B B B A B B B B
//        ^
// Will detect the marked 'B' as an epoch with a period of 1.
// For that reason stable period requirement for any period < MIN_STABLE_PERIOD
// will be rounded up to:
//    MIN_STABLE_PERIOD x STABLE_PERIOD_HYSTERESIS
#define MIN_STABLE_PERIOD 4
// This defines the criteria for changing state from PERIOD_DETECTED to
// WAITING. If the period is not the detected period (stored in _last_period)
// for _last_period x UNSTABLE_PERIOD_HYSTERESIS records we will go back to
// WAITING state.
// Having a hysteresis allows for rare erraneous insertions or missing records
// in a period.
#define UNSTABLE_PERIOD_HYSTERESIS 1.5


namespace geopm
{
    EditDistEpochRecordFilter::EditDistEpochRecordFilter(int history_buffer_size)
        : EditDistEpochRecordFilter(std::make_shared<EditDistPeriodicityDetector>(history_buffer_size))
    {

    }

    EditDistEpochRecordFilter::EditDistEpochRecordFilter(std::shared_ptr<EditDistPeriodicityDetector> edpd)
        : m_edpd(edpd)
        , m_last_period(-1)
        , m_period_stable(0)
        , m_period_unstable(0)
        , m_last_epoch(-1)
        , m_epoch_count(0)
        , m_record_count(0)
        , m_state(WAITING)
    {

    }

/*
    EditDistEpochRecordFilter::EditDistEpochRecordFilter(const std::string &filter_name)
    {

    }
*/

    std::vector<record_s> EditDistEpochRecordFilter::filter(const record_s &record)
    {
        std::vector<record_s> result;
        // EVENT_EPOCH_COUNT needs to be filtered but everything else passes through.
        if (record.event != EVENT_EPOCH_COUNT) {
            result.push_back(record);
            if (record.event == EVENT_REGION_ENTRY) {
                m_edpd->update(record);
                m_record_count++;
                if (epoch_detector()) {
                    m_epoch_count++;
                    record_s epoch_event = record;
                    epoch_event.event = EVENT_EPOCH_COUNT;
                    epoch_event.signal = m_epoch_count;
                    result.push_back(epoch_event);
                }
            }
        }
        return result;
    }


    bool EditDistEpochRecordFilter::epoch_detector()
    {
        if(m_state == WAITING) {
            // STATE: WAITING

            if(m_edpd->get_score() >= m_edpd->get_period()) {
                // If the score is the same as the period or greater the detected period is really low quality.
                // For example: A B C D ... will give period = 1 with score = 1.
                // In that case, we reset the period detection, i.e., we don;t even treat the
                // last period detected as valid.
                m_last_period = -1;
                m_period_stable = 0;
            } else if(m_edpd->get_period() == m_last_period) {
                // Now we have a repeating pattern...
                m_period_stable += 1;
            } else {
                // No repeating pattern but we wtore the current period for future possibility.
                m_last_period = m_edpd->get_period();
                m_period_stable = 0;
            }

            if( ( m_edpd->get_period() <= MIN_STABLE_PERIOD && (m_period_stable == STABLE_PERIOD_HYSTERESIS * MIN_STABLE_PERIOD) ) ||
                ( m_edpd->get_period() > MIN_STABLE_PERIOD && (m_period_stable == STABLE_PERIOD_HYSTERESIS * m_edpd->get_period()) ) )
            {
                // To understand this criteria read MIN_STABLE_PERIOD and STABLE_PERIOD_HYSTERESIS documentation.

                m_state = PERIOD_DETECTED;
                m_last_epoch = m_record_count;
                 // Reset for next use
                m_period_stable = 0;

                return true;
            }
        } else {
            // STATE: PERIOD_DETECTED

            if(m_edpd->get_period() == m_last_period) {
                m_period_unstable = 0;
            } else {
                m_period_unstable += 1;
            }

            if(m_period_unstable == (int)(UNSTABLE_PERIOD_HYSTERESIS *  m_edpd->get_period())) {
                m_state = WAITING;
                 // Reset for next use
                m_period_unstable = 0;
                return false;
            }

            // Note that the following statement may work even if there is insertions, but that should be tested.
            // TODO: We need to evaluate the value of passing the following condition:
            //       (m_record_count - m_last_epoch) > m_edpd->get_period()

            // We fail the following condition:
            //       m_edpd->get_period() < m_last_period
            // which causes a bunch of period=1 s to pass.

            // An idea to be evaluated: What is get_period returns an array of string lengths with different scores
            // Definitely skip length=1 but can move on to another length with a slightly higher score.

            if(m_edpd->get_period() >= m_last_period && (m_record_count - m_last_epoch) >= m_edpd->get_period()) {
                m_last_epoch = m_record_count;
                return true;
            }
        }

        return false;
    }


    int EditDistEpochRecordFilter::parse_name(const std::string &name)
    {
        throw Exception("EditDistEpochRecordFilter::parse_name(): Not implemented",
                         GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return 0;
    }
}
