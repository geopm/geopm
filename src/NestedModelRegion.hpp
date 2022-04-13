/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NESTEDMODELREGION_HPP_INCLUDE
#define NESTEDMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"
#include "SpinModelRegion.hpp"
#include "All2allModelRegion.hpp"

namespace geopm
{
    class NestedModelRegion : public ModelRegion
    {
        public:
            NestedModelRegion(double big_o_in,
                              int verbosity,
                              bool do_imbalance,
                              bool do_progress,
                              bool do_unmarked);
            virtual ~NestedModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            /// @todo Make these std::unique_ptr and forward delclare
            SpinModelRegion m_spin_region;
            All2allModelRegion m_all2all_region;
    };
}

#endif
