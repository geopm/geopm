/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
            enum m_fallback_e {
                M_FALLBACK_MSRSAFE,
                M_FALLBACK_MSR,
                M_NUM_FALLBACK,
            };

            MSRPath() = default;
            virtual ~MSRPath() = default;
            virtual std::string msr_path(int cpu_idx,
                                         int fallback_idx);
            virtual std::string msr_batch_path(void);
    };
}

#endif
