/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BARRIERMODELREGION_HPP_INCLUDE
#define BARRIERMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"

namespace geopm
{
    class BarrierModelRegion : public ModelRegion
    {
        public:
            BarrierModelRegion(double big_o_in,
                               int verbosity,
                               bool do_imbalance,
                               bool do_progress,
                               bool do_unmarked);
            virtual ~BarrierModelRegion() = default;
            void big_o(double big_o);
            void run(void);
        private:
            bool m_is_mpi_enabled;
    };
}

#endif
