/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef POLICYTRACER_HPP_INCLUDE
#define POLICYTRACER_HPP_INCLUDE

#include <vector>
#include <memory>
#include <string>

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
    };
}

#endif
