/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "BarrierModelRegion.hpp"

#include <stdlib.h>
#include <mpi.h>
#include <iostream>

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

    }

    void BarrierModelRegion::big_o(double big_o)
    {

    }

    void BarrierModelRegion::run(void)
    {
        if (!getenv("GEOPMBENCH_NO_MPI")) {
            if (m_verbosity != 0) {
                std::cout << "Executing barrier\n";
            }
            int err = MPI_Barrier(MPI_COMM_WORLD);
            if (err) {
                throw Exception("MPI_Barrier", err, __FILE__, __LINE__);
            }
        }
    }
}
