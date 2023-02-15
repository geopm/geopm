/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPINMODELREGION_HPP_INCLUDE
#define SPINMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"

namespace geopm
{
    class SpinModelRegion : public ModelRegion
    {
        public:
            SpinModelRegion(double big_o_in,
                            int verbosity,
                            bool do_imbalance,
                            bool do_progress,
                            bool do_unmarked);
            virtual ~SpinModelRegion();
            void big_o(double big_o);
            void run(void);
            virtual void run_atom(void);
        protected:
            double m_delay;
    };
}

#endif
