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
#include "IgnoreModelRegion.hpp"

#include <iostream>

#include "geopm.h"
#include "Exception.hpp"

namespace geopm
{

    IgnoreModelRegion::IgnoreModelRegion(double big_o_in,
                                         int verbosity,
                                         bool do_imbalance,
                                         bool do_progress,
                                         bool do_unmarked)
        : ModelRegion(verbosity)
    {
        m_name = "ignore";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        big_o(big_o_in);
        int err = ModelRegion::region(GEOPM_REGION_HINT_IGNORE);
        if (err) {
            throw Exception("IgnoreModelRegion::IgnoreModelRegion()",
                            err, __FILE__, __LINE__);
        }
    }

    IgnoreModelRegion::~IgnoreModelRegion()
    {

    }

    void IgnoreModelRegion::big_o(double big_o_in)
    {
        num_progress_updates(big_o_in);
        double seconds = big_o_in / m_num_progress_updates;
        m_delay = {(time_t)(seconds),
                   (long)((seconds - (time_t)(seconds)) * 1E9)};

        m_big_o = big_o_in;
    }

    void IgnoreModelRegion::run(void)
    {
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing ignored " << m_big_o << " second sleep."  << std::endl << std::flush;
            }
            ModelRegion::region_enter();
            for (uint64_t i = 0 ; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);

                int err;
                err = clock_nanosleep(CLOCK_REALTIME, 0, &m_delay, NULL);
                if (err) {
                    throw Exception("IgnoreModelRegion::run()",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
