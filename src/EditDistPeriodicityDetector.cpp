/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "EditDistPeriodicityDetector.hpp"

#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>

#include "record.hpp"
#include "geopm_debug.hpp"


namespace geopm
{
    EditDistPeriodicityDetector::EditDistPeriodicityDetector(int history_buffer_size)
        : m_history_buffer(history_buffer_size)
        , m_history_buffer_size(history_buffer_size)
        , m_period(-1)
        , m_score(-1)
        , m_record_count(0)
        , m_DP(history_buffer_size * history_buffer_size * history_buffer_size)
    {

    }

    void EditDistPeriodicityDetector::update(const record_s &record)
    {
        if (record.event == EVENT_REGION_ENTRY) {
            m_history_buffer.insert(record.signal);
            ++m_record_count;
            calc_period();
        }
    }

    size_t EditDistPeriodicityDetector::Didx(int ii, int jj, int mm) const
    {
        return (ii % m_history_buffer_size) * m_history_buffer_size * m_history_buffer_size +
               (jj % m_history_buffer_size) * m_history_buffer_size +
               (mm % m_history_buffer_size);
    }

    void EditDistPeriodicityDetector::Dset(int ii, int jj, int mm, uint32_t val)
    {
        m_DP[Didx(ii, jj, mm)] = val;
    }

    uint32_t EditDistPeriodicityDetector::Dget(int ii, int jj, int mm) const
    {
        // This value is supposed to be INF but not so large that it gets wrapped around when
        // a small value is added to it.
        uint32_t result = std::numeric_limits<uint32_t>::max() / 2;

        // D[ii, jj, mm] is the string-edit distance between records [0..ii) and
        // [mm..mm+jj). If ii is too short, the values will be truncated.
        // Likewise, if mm is too small, this refers to data that has been lost.
        // We want to truncate values of jj that are too *large*.
        if (m_record_count - ii < m_history_buffer_size &&
            jj < m_history_buffer_size &&
            m_record_count - mm < m_history_buffer_size) {
            result = m_DP[Didx(ii, jj, mm)];
        }

        return result;
    }

    void EditDistPeriodicityDetector::calc_period(void)
    {
        if (m_record_count < 2) {
            return;
        }

        int num_recs_in_hist = m_history_buffer.size();

        for (int ii = std::max({0, m_record_count - m_history_buffer_size}); ii < m_record_count; ++ii) {
            Dset(ii, 0, m_record_count - 1, 0);
        }
        for (int mm = std::max({0, m_record_count - m_history_buffer_size}); mm < m_record_count; ++mm) {
            Dset(0, m_record_count - mm, mm, m_record_count - mm);
        }

        uint64_t last_rec_in_history = m_history_buffer.value(num_recs_in_hist - 1);
        for (int mm = std::max({1, m_record_count - m_history_buffer_size}); mm < m_record_count; ++mm) {
            for (int ii = std::max({1, m_record_count - m_history_buffer_size}); ii < mm + 1; ++ii) {
                // If the record to be compared to the latest addition is not new enough to reside in the
                // history buffer, by default it is not a match. If it is in the history buffer, the penalty
                // term is 0 if they are equal.

                // ii is the length of the first substring that we are comparing against. If there were
                // no history truncation, we would be comparing entry ii - 1 (0-indexed) to the latest
                // record, entry m_record_count - 1.

                // entry_age is 1 for the most recent entry (it goes from 1 to m_record_count, inclusive)
                int entry_age = m_record_count - (ii - 1);
                // If entry_age is above num_recs_in_hist, it will no longer be in our buffer.
                bool cond_newrec = entry_age <= num_recs_in_hist;
                int term = 2;
                if (cond_newrec) {
                    // We still have the desired record, so we can compare it to the new one.
                    uint64_t compared_rec = m_history_buffer.value(num_recs_in_hist - entry_age);
                    if (compared_rec == last_rec_in_history) {
                        term = 0;
                    }
                }
                // The value that will go into the D matrix (i.e. penalty) is the minimum of the
                // added penalties from all directions (add/subtract/replace).
                uint32_t d_value = std::min({Dget(ii - 1, m_record_count - mm, mm) + 1,
                                             Dget(ii, m_record_count - mm - 1, mm) + 1,
                                             Dget(ii - 1, m_record_count - mm - 1, mm) + term});
                Dset(ii, m_record_count - mm, mm, d_value);
            }
        }

        int mm = std::max({(int)(m_record_count / 2.0 + 0.5), m_record_count - m_history_buffer_size});
        int bestm = mm;
        uint32_t bestval = Dget(mm, m_record_count - mm, mm);
        ++mm;
        for(; mm < m_record_count; ++mm) {
            uint32_t val = Dget(mm, m_record_count - mm, mm);
            if(val < bestval) {
                bestval = val;
                bestm = mm;
            }
        }

        m_score = bestval;
        // Originally this was:
        //      m_period = n - bestm + 1;
        // However since the algorithm find the bestm with the lowest index it will
        // return a string with a repeating pattern in it. For example:
        //     A B A B A B ...
        // find_min_match will find the smallest repeating pattern in it: A B
        size_t bestm_reverse_index = m_record_count - bestm;
        m_period = find_smallest_repeating_pattern(num_recs_in_hist - bestm_reverse_index);
    }

    int EditDistPeriodicityDetector::get_period(void) const
    {
        return m_period;
    }

    int EditDistPeriodicityDetector::get_score(void) const
    {
        return m_score;
    }

    uint64_t EditDistPeriodicityDetector::get_history_value(int index) const
    {
        return m_history_buffer.value(index - 1);
    }

    int EditDistPeriodicityDetector::num_records(void) const
    {
        return m_record_count;
    }

    int EditDistPeriodicityDetector::find_smallest_repeating_pattern(int slice_start) const
    {
        if (m_history_buffer.size() == slice_start) {
            return 1;
        }
        std::vector<uint64_t> recs = m_history_buffer.make_vector(
                                         slice_start, m_history_buffer.size());

        int result = recs.size();
        bool perfect_match = false;
        int div_max = (recs.size() / 2) + 1;
        for (int div = 1; !perfect_match && div < div_max; ++div) {
            if (recs.size() % div == 0) {
                perfect_match = true;
                int group_max = recs.size() / div;
                for (int group = 1; perfect_match && group < group_max; ++group) {
                    int curr = div * group;
                    auto cmp1_begin = recs.begin() + curr - div;
                    auto cmp1_end = recs.begin() + curr;
                    auto cmp2_begin = cmp1_end;
                    if (!std::equal(cmp1_begin, cmp1_end, cmp2_begin)) {
                        perfect_match = false;
                    }
                }
                if (perfect_match) {
                    result = div;
                }
            }
        }
        return result;
    }
}
