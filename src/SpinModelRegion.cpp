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
#include "SpinModelRegion.hpp"

#include <iostream>

#include "geopm_time.h"
#include "Exception.hpp"

namespace geopm
{
   SpinModelRegion::SpinModelRegion(double big_o_in,
                                    int verbosity,
                                    bool do_imbalance,
                                    bool do_progress,
                                    bool do_unmarked)
        : ModelRegion(verbosity)
    {
        m_name = "spin";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegion::region();
        if (err) {
            throw Exception("SpinModelRegion::SpinModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    SpinModelRegion::~SpinModelRegion()
    {

    }

    void SpinModelRegion::big_o(double big_o_in)
    {
        num_progress_updates(big_o_in);
        m_delay = big_o_in / m_num_progress_updates;
        m_big_o = big_o_in;
    }

    void SpinModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing " << m_big_o << " second spin."  << std::endl << std::flush;
            }
            ModelRegion::region_enter();
            for (uint64_t i = 0 ; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);

                double timeout = 0.0;
                struct geopm_time_s start = {{0,0}};
                struct geopm_time_s curr = {{0,0}};
                (void)geopm_time(&start);
                while (timeout < m_delay) {
                    (void)geopm_time(&curr);
                    timeout = geopm_time_diff(&start, &curr);
                }

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
