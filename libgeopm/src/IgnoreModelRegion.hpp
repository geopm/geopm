/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IGNOREMODELREGION_HPP_INCLUDE
#define IGNOREMODELREGION_HPP_INCLUDE

#include "geopm/ModelRegion.hpp"

namespace geopm
{
    class IgnoreModelRegion : public ModelRegion
    {
        public:
            IgnoreModelRegion(double big_o_in,
                              int verbosity,
                              bool do_imbalance,
                              bool do_progress,
                              bool do_unmarked);
            virtual ~IgnoreModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            struct timespec m_delay;
    };
}

#endif
