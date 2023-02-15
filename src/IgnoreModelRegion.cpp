/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "IgnoreModelRegion.hpp"

#include <iostream>

#include "geopm_hint.h"
#include "geopm/Exception.hpp"

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
        int err = ModelRegion::region(GEOPM_REGION_HINT_IGNORE);
        if (err) {
            throw Exception("IgnoreModelRegion::IgnoreModelRegion()",
                            err, __FILE__, __LINE__);
        }
        big_o(big_o_in);
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
