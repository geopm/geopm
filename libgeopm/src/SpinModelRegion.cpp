/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SpinModelRegion.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <time.h>

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
                std::cout << "Executing " << m_big_o << " second spin." << std::endl << std::flush;
            }
            ModelRegion::region_enter();

            std::atomic<int> shared_value{0};

            for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                ModelRegion::loop_enter(i);
                // Thread 1: Sleep and update shared_value
                auto worker_thread = std::thread([&shared_value](double delay) {
                    struct timespec sleep_time = {0, 0};
                    sleep_time.tv_sec = static_cast<time_t>(delay);
                    sleep_time.tv_nsec = static_cast<long>((delay - sleep_time.tv_sec) * 1e9);
                    clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, nullptr);
                    shared_value.store(1, std::memory_order_release);
                }, m_delay);

                // Thread 2: Main thread waits for shared_value to change
                while (shared_value.load(std::memory_order_acquire) == 0) {
                    run_atom();
                }

                worker_thread.join();
                ModelRegion::loop_exit();
            }

            ModelRegion::region_exit();
        }
    }
}
