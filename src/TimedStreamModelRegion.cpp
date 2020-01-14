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
#include "TimedStreamModelRegion.hpp"

#include <iostream>

#include "geopm.h"
#include "geopm_time.h"
#include "Exception.hpp"

namespace geopm
{
    TimedStreamModelRegion::TimedStreamModelRegion(double big_o_in,
                                                   int verbosity,
                                                   bool do_imbalance,
                                                   bool do_progress,
                                                   bool do_unmarked)
        : StreamModelRegion(big_o_in, verbosity, do_imbalance, do_progress, do_unmarked)
    {
        m_name = "timed_stream";
        int err = ModelRegion::region(GEOPM_REGION_HINT_MEMORY);
        if (err) {
            throw Exception("TimedStreamModelRegion::TimedStreamModelRegion()",
                            err, __FILE__, __LINE__);
        }

        if (m_verbosity) {
            std::cout << "Calibrating timed_stream region to " << big_o_in << " seconds.  Please wait..." << std::endl;
        }
        double measured_time = 0;
        bool done = false;
        double new_big_o = big_o_in;
        const double EPS = big_o_in / 100.0;
        const int MAX_ITERATIONS = 20;
        int iteration_count = 0;
        // Start with 1:1 ratio: assume big-o == seconds and test.  Use
        // the measured runtime to update the ratio of big-o to time.
        while (!done) {
            big_o(new_big_o);
            // warm caches
            run();
            geopm_time_s start;
            geopm_time(&start);
            run();
            measured_time = geopm_time_since(&start);
            if (m_verbosity) {
                std::cout << "stream big-o=" << new_big_o << ", "
                          << "runtime=" << measured_time << "s" << std::endl;
            }
            new_big_o = new_big_o * big_o_in / measured_time;
            if (m_verbosity) {
                std::cout << "ratio=" << (big_o_in / measured_time) << "; new big-o: " << new_big_o << std::endl;
            }

            if (fabs(measured_time - big_o_in) < EPS) {
                done = true;
            }
            ++iteration_count;
            if (!done && iteration_count == MAX_ITERATIONS) {
                std::cout << "Warning: <geopm> could not find a big-o for requested runtime within " << MAX_ITERATIONS << " iterations. " << std::endl;
                done = true;
            }
        }
        if (m_verbosity) {
            std::cout << "Calibration complete.  Using stream big-o of " << new_big_o << std::endl;
        }
        m_big_o = new_big_o;
    }

   TimedStreamModelRegion::~TimedStreamModelRegion()
    {

    }
}
