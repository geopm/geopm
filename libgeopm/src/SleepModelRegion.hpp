/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SLEEPMODELREGION_HPP_INCLUDE
#define SLEEPMODELREGION_HPP_INCLUDE

#include "geopm/ModelRegion.hpp"

namespace geopm
{
    class SleepModelRegion : public ModelRegion
    {
        public:
            SleepModelRegion(double big_o_in,
                             int verbosity,
                             bool do_imbalance,
                             bool do_progress,
                             bool do_unmarked);
            virtual ~SleepModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            struct timespec m_delay;
    };
}

#endif
