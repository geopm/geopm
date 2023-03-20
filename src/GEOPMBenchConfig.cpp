/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "GEOPMBenchConfig.hpp"

#include <stdlib.h>

namespace geopm
{
    const GEOPMBenchConfig &geopmbench_config(void)
    {
        static GEOPMBenchConfigImp instance;
        return instance;
    }

    GEOPMBenchConfigImp::GEOPMBenchConfigImp()
        : GEOPMBenchConfigImp(getenv("GEOPMBENCH_NO_MPI") == nullptr)
    {
    }

    GEOPMBenchConfigImp::GEOPMBenchConfigImp(bool is_mpi_enabled)
        : m_is_mpi_enabled(is_mpi_enabled)
    {
    }

    bool GEOPMBenchConfigImp::is_mpi_enabled() const
    {
        return m_is_mpi_enabled;
    }
}
