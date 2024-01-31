/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSRPATH_HPP_INCLUDE
#define MSRPATH_HPP_INCLUDE

#include <string>

namespace geopm
{
    class MSRPath
    {
        public:
            MSRPath();
            MSRPath(int driver_type);
            virtual ~MSRPath() = default;
            virtual std::string msr_path(int cpu_idx) const;
            virtual std::string msr_batch_path(void) const;
        private:
            const int m_driver_type;
    };
}

#endif
