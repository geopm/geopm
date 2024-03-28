/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENDPOINTPOLICYTRACER_HPP_INCLUDE
#define ENDPOINTPOLICYTRACER_HPP_INCLUDE

#include <vector>
#include <memory>

namespace geopm
{
    class EndpointPolicyTracer
    {
        public:
            EndpointPolicyTracer() = default;
            virtual ~EndpointPolicyTracer() = default;
            virtual void update(const std::vector<double> &policy) = 0;
            static std::unique_ptr<EndpointPolicyTracer> make_unique(void);
    };
}

#endif
