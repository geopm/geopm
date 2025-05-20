/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DGEMMMODELREGION_HPP_INCLUDE
#define DGEMMMODELREGION_HPP_INCLUDE

#include "geopm/ModelRegion.hpp"

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
            DGEMMModelRegion(const DGEMMModelRegion &other) = delete;
            DGEMMModelRegion &operator=(const DGEMMModelRegion &other) = delete;
            virtual ~DGEMMModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_matrix_a;
            double *m_matrix_b;
            double *m_matrix_c;
            const size_t m_matrix_m_size;
            const size_t m_matrix_n_size;
            const size_t m_matrix_k_size;
            const size_t m_pad_size;
            const int m_num_warmup;
        private:
            void cleanup(void);
            void warmup(void);
            void num_progress_updates(double big_o_in);

            uint64_t m_start_rid;
    };
}

#endif
