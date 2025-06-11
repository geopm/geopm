/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DGEMMGPUMODELREGION_HPP_INCLUDE
#define DGEMMGPUMODELREGION_HPP_INCLUDE

#include <CL/sycl.hpp>
#include "geopm/ModelRegion.hpp"

namespace geopm
{
    class DGEMMGPUModelRegion : public ModelRegion
    {
        public:
            DGEMMGPUModelRegion(double big_o_in,
                                int verbosity,
                                bool do_imbalance,
                                bool do_progress,
                                bool do_unmarked);
            DGEMMGPUModelRegion(const DGEMMGPUModelRegion &other) = delete;
            DGEMMGPUModelRegion &operator=(const DGEMMGPUModelRegion &other) = delete;
            virtual ~DGEMMGPUModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_host_matrix_a;
            double *m_host_matrix_b;
            double *m_host_matrix_c;
            cl::sycl::queue *m_queue;
            cl::sycl::buffer<double, 2> *m_matrix_a_buffer;
            cl::sycl::buffer<double, 2> *m_matrix_b_buffer;
            cl::sycl::buffer<double, 2> *m_matrix_c_buffer;
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
