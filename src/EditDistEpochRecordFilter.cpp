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

namespace geopm
{
    EditDistEpochRecordFilter::EditDistEpochRecordFilter(double stable_period_hysteresis,
                                                         int min_stable_period,
                                                         double unstable_period_hysteresis,
                                                         int history_buffer_size)
        : EditDistEpochRecordFilter(stable_period_hysteresis,
                                    min_stable_period,
                                    unstable_period_hysteresis,
                                    std::make_shared<EditDistPeriodicityDetector>(history_buffer_size))
    {

    }

    EditDistEpochRecordFilter::EditDistEpochRecordFilter(double stable_period_hysteresis,
                                                         int min_stable_period,
                                                         double unstable_period_hysteresis,
                                                         std::shared_ptr<EditDistPeriodicityDetector> edpd)
        : m_stable_period_hysteresis(stable_period_hysteresis)
        , m_min_stable_period(min_stable_period)
        , m_unstable_period_hysteresis(unstable_period_hysteresis)
        , m_edpd(edpd)
        , m_last_period(-1)
        , m_period_stable(0)
        , m_period_unstable(0)
        , m_is_period_detected(false)
        , m_last_epoch(-1)
        , m_epoch_count(0)
        , m_record_count(0)
    {

    }

    std::vector<record_s> EditDistEpochRecordFilter::filter(const record_s &record)
    {
        std::vector<record_s> result;
        // EVENT_EPOCH_COUNT needs to be filtered but everything else passes through.
        if (record.event != EVENT_EPOCH_COUNT) {
            result.push_back(record);
            if (record.event == EVENT_REGION_ENTRY) {
                m_edpd->update(record);
                m_record_count++;
                if (epoch_detected()) {
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


    bool EditDistEpochRecordFilter::epoch_detected()
    {
        if (!m_is_period_detected) {
            if (m_edpd->get_score() >= m_edpd->get_period()) {
                // If the score is the same as the period or greater the detected period is really low quality.
                // For example: A B C D ... will give period = 1 with score = 1.
                // In that case, we reset the period detection, i.e., we don;t even treat the
                // last period detected as valid.
                m_last_period = -1;
                m_period_stable = 0;
            }
            else if (m_edpd->get_period() == m_last_period) {
                // Now we have a repeating pattern...
                m_period_stable += 1;
            }
            else {
                // No repeating pattern but we store the current period for future possibility.
                m_last_period = m_edpd->get_period();
                m_period_stable = 0;
            }

            if ((m_edpd->get_period() <= m_min_stable_period &&
                 (m_period_stable == m_stable_period_hysteresis * m_min_stable_period)) ||
                (m_edpd->get_period() > m_min_stable_period &&
                 (m_period_stable == m_stable_period_hysteresis * m_edpd->get_period()))) {
                // To understand this criteria read m_min_stable_period and m_stable_period_hysteresis documentation.

                m_is_period_detected = true;
                m_last_epoch = m_record_count;
                // Reset for next use
                m_period_stable = 0;

                return true;
            }
        }
        else {
            // PERIOD_DETECTED

            if (m_edpd->get_period() == m_last_period) {
                m_period_unstable = 0;
            }
            else {
                m_period_unstable += 1;
            }

            if (m_period_unstable == (int)(m_unstable_period_hysteresis *  m_edpd->get_period())) {
                m_is_period_detected = false;
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

            if (m_edpd->get_period() >= m_last_period && (m_record_count - m_last_epoch) >= m_edpd->get_period()) {
                m_last_epoch = m_record_count;
                return true;
            }
        }
        return false;
    }
}
