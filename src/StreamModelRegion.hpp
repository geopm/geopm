/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STREAMMODELREGION_HPP_INCLUDE
#define STREAMMODELREGION_HPP_INCLUDE

#include "ModelRegion.hpp"

namespace geopm
{
    class StreamModelRegion : public ModelRegion
    {
        public:
            StreamModelRegion(double big_o_in,
                              int verbosity,
                              bool do_imbalance,
                              bool do_progress,
                              bool do_unmarked);
            StreamModelRegion(const StreamModelRegion &other) = delete;
            StreamModelRegion &operator=(const StreamModelRegion &other) = delete;
            virtual ~StreamModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_array_a;
            double *m_array_b;
            double *m_array_c;
            size_t m_array_len;
            const size_t m_align;
        private:
            void cleanup(void);
    };
}

#endif
