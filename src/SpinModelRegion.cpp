/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "SpinModelRegion.hpp"

#include <iostream>

#include "geopm_time.h"
#include "geopm/Exception.hpp"

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
        ModelRegion::region();
        big_o(big_o_in);
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

    void SpinModelRegion::run_atom(void)
    {

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
                    run_atom();
                    (void)geopm_time(&curr);
                    timeout = geopm_time_diff(&start, &curr);
                }

                ModelRegion::loop_exit();
            }
            ModelRegion::region_exit();
        }
    }
}
