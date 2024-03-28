/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "TimedScalingModelRegion.hpp"
#include "ScalingModelRegion.hpp"

namespace geopm
{
    TimedScalingModelRegion::TimedScalingModelRegion(double big_o_in,
                                                     int verbosity,
                                                     bool do_imbalance,
                                                     bool do_progress,
                                                     bool do_unmarked)
        : SpinModelRegion(big_o_in, verbosity, do_imbalance, do_progress, do_unmarked)
        , m_scaling_model(std::make_shared<ScalingModelRegion>(1, 0, false, false, true))
    {

    }

    void TimedScalingModelRegion::run_atom(void)
    {
        m_scaling_model->run_atom();
    }
}
