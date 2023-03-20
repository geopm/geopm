/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "BarrierModelRegion.hpp"

#include <mpi.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "GEOPMBenchConfig.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{
    BarrierModelRegion::BarrierModelRegion(double big_o_in,
                                           int verbosity,
                                           bool do_imbalance,
                                           bool do_progress,
                                           bool do_unmarked)
        : ModelRegion(verbosity)
    {
        const GEOPMBenchConfig &config = geopmbench_config();
        m_is_mpi_enabled = config.is_mpi_enabled();
    }

    void BarrierModelRegion::big_o(double big_o)
    {

    }

    void BarrierModelRegion::run(void)
    {
        if (m_is_mpi_enabled) {
            if (m_verbosity != 0) {
                std::cout << "Executing barrier\n";
            }
            int err = MPI_Barrier(MPI_COMM_WORLD);
            if (err) {
                throw Exception("MPI_Barrier", err, __FILE__, __LINE__);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}
