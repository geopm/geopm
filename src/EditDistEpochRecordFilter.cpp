/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <exception>

#include "EditDistEpochRecordFilter.hpp"
#include "EditDistPeriodicityDetector.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "record.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    static int parse_history_buffer_size(const std::string &name)
    {
        int history_buffer_size;
        int min_hysteresis_base_period;
        int min_detectable_period;
        double stable_period_hysteresis;
        double unstable_period_hysteresis;
        EditDistEpochRecordFilter::parse_name(name,
                                              history_buffer_size,
                                              min_hysteresis_base_period,
                                              min_detectable_period,
                                              stable_period_hysteresis,
                                              unstable_period_hysteresis);
        return history_buffer_size;
    }

    static int parse_min_hysteresis_base_period(const std::string &name)
    {
        int history_buffer_size;
        int min_hysteresis_base_period;
        int min_detectable_period;
        double stable_period_hysteresis;
        double unstable_period_hysteresis;
        EditDistEpochRecordFilter::parse_name(name,
                                              history_buffer_size,
                                              min_hysteresis_base_period,
                                              min_detectable_period,
                                              stable_period_hysteresis,
                                              unstable_period_hysteresis);
        return min_hysteresis_base_period;
    }

    static int parse_min_detectable_period(const std::string &name)
    {
        int history_buffer_size;
        int min_hysteresis_base_period;
        int min_detectable_period;
        double stable_period_hysteresis;
        double unstable_period_hysteresis;
        EditDistEpochRecordFilter::parse_name(name,
                                              history_buffer_size,
                                              min_hysteresis_base_period,
                                              min_detectable_period,
                                              stable_period_hysteresis,
                                              unstable_period_hysteresis);
        return min_detectable_period;
    }

    static int parse_stable_period_hysteresis(const std::string &name)
    {
        int history_buffer_size;
        int min_hysteresis_base_period;
        int min_detectable_period;
        double stable_period_hysteresis;
        double unstable_period_hysteresis;
        EditDistEpochRecordFilter::parse_name(name,
                                              history_buffer_size,
                                              min_hysteresis_base_period,
                                              min_detectable_period,
                                              stable_period_hysteresis,
                                              unstable_period_hysteresis);
        return stable_period_hysteresis;
    }

    static int parse_unstable_period_hysteresis(const std::string &name)
    {
        int history_buffer_size;
        int min_hysteresis_base_period;
        int min_detectable_period;
        double stable_period_hysteresis;
        double unstable_period_hysteresis;
        EditDistEpochRecordFilter::parse_name(name,
                                              history_buffer_size,
                                              min_hysteresis_base_period,
                                              min_detectable_period,
                                              stable_period_hysteresis,
                                              unstable_period_hysteresis);
        return unstable_period_hysteresis;
    }

    EditDistEpochRecordFilter::EditDistEpochRecordFilter(const std::string &name)
        : EditDistEpochRecordFilter(parse_history_buffer_size(name),
                                    parse_min_hysteresis_base_period(name),
                                    parse_min_detectable_period(name),
                                    parse_stable_period_hysteresis(name),
                                    parse_unstable_period_hysteresis(name))
    {

    }

    EditDistEpochRecordFilter::EditDistEpochRecordFilter(int history_buffer_size,
                                                         int min_hysteresis_base_period,
                                                         int min_detectable_period,
                                                         double stable_period_hysteresis,
                                                         double unstable_period_hysteresis)
        : EditDistEpochRecordFilter(std::make_shared<EditDistPeriodicityDetector>(history_buffer_size),
                                    min_hysteresis_base_period,
                                    min_detectable_period,
                                    stable_period_hysteresis,
                                    unstable_period_hysteresis)
    {

    }

    EditDistEpochRecordFilter::EditDistEpochRecordFilter(std::shared_ptr<EditDistPeriodicityDetector> edpd,
                                                         int min_hysteresis_base_period,
                                                         int min_detectable_period,
                                                         double stable_period_hysteresis,
                                                         double unstable_period_hysteresis)
        : m_edpd(std::move(edpd))
        , m_min_hysteresis_base_period(min_hysteresis_base_period)
        , m_min_detectable_period(min_detectable_period)
        , m_stable_period_hysteresis(stable_period_hysteresis)
        , m_unstable_period_hysteresis(unstable_period_hysteresis)
        , m_last_period(-1)
        , m_period_stable(0)
        , m_period_unstable(0)
        , m_is_period_detected(false)
        , m_last_epoch(-1)
        , m_epoch_count(0)
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
        // The below IF statement is a state machine where the state variable is m_is_period_detected
        // (true means state PERIOD_DETECTED false means NO_PERIOD_DETECTED).
        //
        if (!m_is_period_detected) {
            if (m_edpd->get_score() >= m_edpd->get_period() || m_edpd->get_period() < m_min_detectable_period) {
                // If the score is the same as the period or greater the detected period is really low quality.
                // For example: A B C D ... will give period = 1 with score = 1.
                // In that case, we reset the period detection, i.e., we don't even treat the
                // last period detected as valid.
                // Also periods that are too short don't count even if they are good quality.
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

            if ((m_edpd->get_period() <= m_min_hysteresis_base_period &&
                 (m_period_stable == m_stable_period_hysteresis * m_min_hysteresis_base_period)) ||
                (m_edpd->get_period() > m_min_hysteresis_base_period &&
                 (m_period_stable >= (int)(m_stable_period_hysteresis * m_edpd->get_period())))) {
                // To understand this criteria read m_min_hysteresis_base_period and m_stable_period_hysteresis documentation.

                m_is_period_detected = true;
                m_last_epoch = m_edpd->num_records();
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

            if (m_period_unstable >= (int)(m_unstable_period_hysteresis * m_last_period)) {
                m_is_period_detected = false;
                // Reset for next use
                m_period_unstable = 0;
                return false;
            }

            // Note that the following statement may work even if there is insertions, but that should be tested.
            // @todo We need to evaluate the value of passing the following condition:
            //       (m_edpd->num_records() - m_last_epoch) > m_edpd->get_period()

            // We fail the following condition:
            //       m_edpd->get_period() < m_last_period
            // which causes a bunch of period=1 s to pass.

            // An idea to be evaluated: What is get_period returns an array of string lengths with different scores
            // Definitely skip length=1 but can move on to another length with a slightly higher score.

            if (m_edpd->get_period() >= m_last_period && (m_edpd->num_records() - m_last_epoch) >= m_edpd->get_period()) {
                m_last_epoch = m_edpd->num_records();
                return true;
            }
        }
        return false;
    }

    void EditDistEpochRecordFilter::parse_name(const std::string &name,
                                               int &history_buffer_size,
                                               int &min_hysteresis_base_period,
                                               int &min_detectable_period,
                                               double &stable_period_hysteresis,
                                               double &unstable_period_hysteresis)
    {
        auto pieces = string_split(name, ",");
        GEOPM_DEBUG_ASSERT(pieces.size() > 0, "string_split() failed.");

        // empirically determined default values
        history_buffer_size = 50;
        min_hysteresis_base_period = 4;
        min_detectable_period = 3;
        stable_period_hysteresis = 1.0;
        unstable_period_hysteresis = 1.5;

        if (pieces[0] != "edit_distance") {
            throw Exception("Unknown filter name", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (pieces.size() > 1) {
            try {
                history_buffer_size = std::stoi(pieces[1]);
            }
            catch (const std::exception &ex) {
                throw Exception("EditDistEpochRecordFilter::parse_name(): invalid buffer size",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (pieces.size() > 2) {
            try {
                min_hysteresis_base_period = std::stoi(pieces[2]);
            }
            catch (const std::exception &ex) {
                throw Exception("EditDistEpochRecordFilter::parse_name(): invalid hysteresis base period",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (pieces.size() > 3) {
            try {
                min_detectable_period = std::stoi(pieces[3]);
            }
            catch (const std::exception &ex) {
                throw Exception("EditDistEpochRecordFilter::parse_name(): invalid minimum detectable period",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (pieces.size() > 4) {
            try {
                stable_period_hysteresis = std::stod(pieces[4]);
            }
            catch (const std::exception &ex) {
                throw Exception("EditDistEpochRecordFilter::parse_name(): invalid stable hysteresis",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (pieces.size() > 5) {
            try {
                unstable_period_hysteresis = std::stod(pieces[5]);
            }
            catch (const std::exception &ex) {
                throw Exception("EditDistEpochRecordFilter::parse_name(): invalid unstable hysteresis",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        if (pieces.size() > 6) {
            throw Exception("Too many commas in filter name", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
}
