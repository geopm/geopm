/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DGEMMMODELREGION_HPP_INCLUDE
#define DGEMMMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"

namespace geopm
{
    class DGEMMModelRegion : public ModelRegion
    {
        public:
            DGEMMModelRegion(double big_o_in,
                             int verbosity,
                             bool do_imbalance,
                             bool do_progress,
                             bool do_unmarked);
            virtual ~DGEMMModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_matrix_a;
            double *m_matrix_b;
            double *m_matrix_c;
            size_t m_matrix_size;
            const size_t m_pad_size;
            const int m_num_warmup;
        private:
            void cleanup(void);
            void warmup(void);

            uint64_t m_start_rid;
    };
}

#endif
