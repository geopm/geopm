/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MODELAPPLICATION_HPP_INCLUDE
#define MODELAPPLICATION_HPP_INCLUDE

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace geopm
{
    class ModelRegion;

    class ModelApplication
    {
        public:
            ModelApplication(uint64_t loop_count, const std::vector<std::string> &region_name,
                             const std::vector<double> &big_o, int verbosity, int rank);
            virtual ~ModelApplication() = default;
            void run(void);
        protected:
            uint64_t m_repeat;
            int m_rank;
            std::vector<std::shared_ptr<ModelRegion> > m_region;
    };

}

#endif
