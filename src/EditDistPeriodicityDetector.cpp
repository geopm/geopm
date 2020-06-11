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

#include "EditDistPeriodicityDetector.hpp"
#include "record.hpp"

#include "geopm_debug.hpp"

#include <vector>
#include <cmath>

#include <iostream>

using namespace std;

namespace geopm
{
    EditDistPeriodicityDetector::EditDistPeriodicityDetector(unsigned int history_buffer_size)
    : m_history_buffer(history_buffer_size)
    {
    }

    void EditDistPeriodicityDetector::update(const record_s &record)
    {
        if(record.event == EVENT_REGION_ENTRY) {
             m_history_buffer.insert(record.signal);
             calc_period();
         }
    }

    void EditDistPeriodicityDetector::calc_period()
    {
        int n = m_history_buffer.size();

        int D[m_history_buffer.capacity()][m_history_buffer.capacity()+1][m_history_buffer.capacity()+1];
        for(int m=0; m < n+1; m++) {
            for(int i=0; i < n; i++) {
                D[i][0][m] = 0;
                D[0][i][m] = i;
            }
        }

        for(int m=1; m < n+1; m++) {
            for(int i=1; i < m; i++) {
                for(int j=1; j < n-m+2; j++) {
                    int notsame = get_history_value(i) != get_history_value(m+j-1);
                    // Maybe there is a min function with variadic arguments that I don't know about :P
                    // Put the three in a std::vector and pass the vector to min.
                    // std::min({D[i-1][j][m]+1, D[i][j-1][m]+1, D[i-1][j-1][m] + 2*notsame});
                    D[i][j][m] = min(D[i-1][j][m]+1, min(D[i][j-1][m]+1, D[i-1][j-1][m] + 2*notsame));
                }
            }
        }
        int bestm = -1;
        // -1 in this context means "None" since values of bestval are always >= 0
        int bestval = -1;
        for(int m=ceil(n/2.0)+1; m < n+1; m++) {
            if((bestval == -1) or (D[m-1][n-m+1][m] < bestval)) {
                bestval = D[m-1][n-m+1][m];
                bestm = m;
            }
        }
        if(n > 1) {
            m_score = bestval;
            // Originally this was:
            //      m_period = n - bestm + 1;
            // However since the algorithm find the bestm with the lowest index it will
            // return a string with a repeating pattern in it. For example:
            //     A B A B A B ...
            // find_gcd will find the smallest repeating pattern in it: A B
            m_period = find_gcd(bestm);
        } else {
            m_score = -1;
            m_period = -1;
        }

    }

    int EditDistPeriodicityDetector::get_period() const
    {
        return m_period;
    }

    int EditDistPeriodicityDetector::get_score() const
    {
        return m_score;
    }

    void EditDistPeriodicityDetector::reset()
    {
        m_history_buffer.clear();
    }

    uint64_t EditDistPeriodicityDetector::get_history_value(unsigned int index) const
    {
        return m_history_buffer.value(index-1);
    }

    /// Slice from start to the end of history
    /// start incusive
    vector<uint64_t> EditDistPeriodicityDetector::get_history_slice(unsigned int start) const
    {
        return m_history_buffer.make_vector_slice(start-1, m_history_buffer.size());
    }

    int EditDistPeriodicityDetector::find_gcd(unsigned int slice_start) const
    {
        // TODO: This can be optimized further. Especially since we know if period 1, cost is 0.
        if((m_history_buffer.size() - slice_start + 1) == 1)
            return 1;

        vector<uint64_t> recs = get_history_slice(slice_start);

        for(int div = 1; div < (int)(recs.size()/2)+1; div++) {
            if(recs.size() % div != 0) {
                continue;
            } else {
                int start=0;
                bool perfect_match = true;
                for(int group = 1; group < (int)(recs.size()/div); group++) {
                    vector<uint64_t> cmp1(recs.begin() + div * (group - 1), recs.begin() + div * group);
                    vector<uint64_t> cmp2(recs.begin() + div * group, recs.begin() + div * (group + 1));
                    if(cmp1 != cmp2) {
                        perfect_match = false;
                        break;
                    }
                }
                if(perfect_match) {
                    return div;
                }
            }
        }
        return recs.size();
    }
}
