/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REDUCEMODELREGION_HPP_INCLUDE
#define REDUCEMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"

#include <vector>

namespace geopm
{
    class ReduceModelRegion : public ModelRegion
    {
        public:
            ReduceModelRegion(double big_o_in,
                              int verbosity,
                              bool do_imbalance,
                              bool do_progress,
                              bool do_unmarked);
            virtual ~ReduceModelRegion() = default;
            void big_o(double big_o);
            void run(void);
        private:
            int m_num_elem;
            std::vector<double> m_send_buffer;
            std::vector<double> m_recv_buffer;
            bool m_is_mpi_enabled;
    };
}

#endif
