/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ALL2ALLMODELREGION_HPP_INCLUDE
#define ALL2ALLMODELREGION_HPP_INCLUDE

#include "geopm/ModelRegion.hpp"

namespace geopm
{
    class All2allModelRegion : public ModelRegion
    {
        public:
            All2allModelRegion(double big_o_in,
                               int verbosity,
                               bool do_imbalance,
                               bool do_progress,
                               bool do_unmarked);
            All2allModelRegion(const All2allModelRegion &other) = delete;
            All2allModelRegion &operator=(const All2allModelRegion &other) = delete;
            virtual ~All2allModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            char *m_send_buffer;
            char *m_recv_buffer;
            size_t m_num_send;
            int m_num_rank;
            const size_t m_align;
            int m_rank;
            bool m_is_mpi_enabled;
        private:
            void cleanup(void);
    };
}

#endif
