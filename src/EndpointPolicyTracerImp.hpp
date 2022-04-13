/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENDPOINTPOLICYTRACERIMP_HPP_INCLUDE
#define ENDPOINTPOLICYTRACERIMP_HPP_INCLUDE

#include <string>

#include "EndpointPolicyTracer.hpp"

namespace geopm
{

    class CSV;
    class PlatformIO;

    class EndpointPolicyTracerImp : public EndpointPolicyTracer
    {
        public:
            EndpointPolicyTracerImp();
            EndpointPolicyTracerImp(size_t buffer_size,
                                    bool is_trace_enabled,
                                    const std::string &file_name,
                                    PlatformIO &platform_io,
                                    const std::vector<std::string> &policy_names);
            virtual ~EndpointPolicyTracerImp();
            void update(const std::vector<double> &policy);
        private:
            bool m_is_trace_enabled;
            std::unique_ptr<CSV> m_csv;
            PlatformIO &m_platform_io;
            int m_time_signal;
            std::vector<double> m_values;
            int m_num_policy;
    };
}

#endif
