/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef APPLICATIONIO_HPP_INCLUDE
#define APPLICATIONIO_HPP_INCLUDE

#include <cstdint>
#include <set>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <list>

#include "geopm_internal.h"

namespace geopm
{
    class Comm;

    class ApplicationIO
    {
        public:
            ApplicationIO() = default;
            virtual ~ApplicationIO() = default;
            /// @brief Connect to the application via
            ///        shared memory.
            virtual void connect(void) = 0;
            /// @brief Returns true if the application has indicated
            ///        it is shutting down.
            virtual bool do_shutdown(void) const = 0;
            /// @brief Returns the path to the report file.
            virtual std::string report_name(void) const = 0;
            /// @brief Returns the profile name to be used in the
            ///        report.
            virtual std::string profile_name(void) const = 0;
            /// @brief Returns the set of region names recorded by the
            ///        application.
            virtual std::set<std::string> region_name_set(void) const = 0;
            /// @brief Signal to the application that the Controller
            ///        is ready to begin receiving samples.
            virtual void controller_ready(void) = 0;
            /// @brief Signal to the application that the Controller
            ///        has failed critically.
            virtual void abort(void) = 0;
    };

    class ApplicationSampler;

    class ApplicationIOImp : public ApplicationIO
    {
        public:
            ApplicationIOImp();
            ApplicationIOImp(ApplicationSampler &application_sampler);
            virtual ~ApplicationIOImp();
            void connect(void) override;
            bool do_shutdown(void) const override;
            std::string report_name(void) const override;
            std::string profile_name(void) const override;
            std::set<std::string> region_name_set(void) const override;
            void controller_ready(void) override;
            void abort(void) override;
        private:
            static constexpr size_t M_SHMEM_REGION_SIZE = 2*1024*1024;

            bool m_is_connected;
            ApplicationSampler &m_application_sampler;
    };
}

#endif
