/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OMPT_HPP_INCLUDE
#define OMPT_HPP_INCLUDE

#include <string>

namespace geopm
{
    class OMPT
    {
        public:
            OMPT() = default;
            virtual ~OMPT() = default;
            static OMPT &ompt(void);
            virtual bool is_enabled(void) = 0;
            virtual void region_enter(const void *function_ptr) = 0;
            virtual void region_exit(const void *function_ptr) = 0;
    };
}

#endif
