/*
 * Copyright (c) 2016, Intel Corporation
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

#ifndef TRACER_HPP_INCLUDE
#define TRACER_HPP_INCLUDE

#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include <geopm_message.h>

namespace geopm
{
    /// @brief Abstract base class for the Tracer object defines the interface.
    class ITracer
    {
        public:
            ITracer() {}
            virtual ~ITracer() {}
            virtual void update(const std::vector <struct geopm_telemetry_message_s> &telemetry) = 0;
            virtual void update(const struct geopm_policy_message_s &policy) = 0;
    };

    /// @brief Class used to write a trace of the telemetry and policy.
    class Tracer : public ITracer
    {
        public:
            /// @brief Tracer constructor.
            Tracer(std::string header);
            /// @brief Tracer destructor, virtual.
            virtual ~Tracer();
            void update(const std::vector <struct geopm_telemetry_message_s> &telemetry);
            void update(const struct geopm_policy_message_s &policy);
        protected:
            std::string m_header;
            std::string m_hostname;
            bool m_is_trace_enabled;
            bool m_do_header;
            std::ofstream m_stream;
            std::ostringstream m_buffer;
            off_t m_buffer_limit;
            struct geopm_time_s m_time_zero;
            struct geopm_policy_message_s m_policy;
    };
}

#endif
