/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TIMEDSCALINGMODELREGION_HPP_INCLUDE
#define TIMEDSCALINGMODELREGION_HPP_INCLUDE

#include <memory>

#include "SpinModelRegion.hpp"

namespace geopm
{
    class ScalingModelRegion;
    class TimedScalingModelRegion : public SpinModelRegion
    {
        public:
            TimedScalingModelRegion(double big_o_in,
                                    int verbosity,
                                    bool do_imbalance,
                                    bool do_progress,
                                    bool do_unmarked);
            virtual ~TimedScalingModelRegion() = default;
            void run_atom(void);
        protected:
            std::shared_ptr<ScalingModelRegion> m_scaling_model;
    };
}

#endif
