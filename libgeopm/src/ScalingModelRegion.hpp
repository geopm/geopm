/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCALINGMODELREGION_HPP_INCLUDE
#define SCALINGMODELREGION_HPP_INCLUDE

#include <vector>
#include <string>
#include <cstdint>
#include "geopm/ModelRegion.hpp"

namespace geopm
{
    class ScalingModelRegion : public ModelRegion
    {
        public:
            ScalingModelRegion(double big_o_in,
                               int verbosity,
                               bool do_imbalance,
                               bool do_progress,
                               bool do_unmarked);
            virtual ~ScalingModelRegion();
            ScalingModelRegion(const ScalingModelRegion &other) = delete;
            ScalingModelRegion &operator=(const ScalingModelRegion &other) = delete;
            void big_o(double big_o);
            void run(void);
            void run_atom(void);
        protected:
            size_t llc_size(void);

            std::string m_sysfs_cache_dir;
            size_t m_llc_slop_size;
            size_t m_element_size;
            size_t m_rank_per_node;
            size_t m_array_len;
            size_t m_num_atom;
            std::vector<double *> m_arrays;
    };
}

#endif
