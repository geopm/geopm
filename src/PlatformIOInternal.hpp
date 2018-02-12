/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef PLATFORMIOINTERNAL_HPP_INCLUDE
#define PLATFORMIOINTERNAL_HPP_INCLUDE

#include <memory>
#include <list>
#include <vector>

#include "PlatformIO.hpp"

namespace geopm
{
    class IOGroup;

    class PlatformIO : public IPlatformIO
    {
        public:
            /// @brief Constructor for the PlatformIO class.
            PlatformIO();
            PlatformIO(std::list<std::unique_ptr<IOGroup> > iogroup_list);
            PlatformIO(const PlatformIO &other) = delete;
            PlatformIO & operator=(const PlatformIO&) = delete;
            /// @brief Virtual destructor for the PlatformIO class.
            virtual ~PlatformIO();
            void register_iogroup(std::unique_ptr<IOGroup> iogroup) override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx) override;
            int push_control(const std::string &control_name,
                             int domain_type,
                             int domain_idx) override;
            int num_signal(void) const override;
            int num_control(void) const override;
            double sample(int signal_idx) override;
            void adjust(int control_idx, double setting) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double read_signal(const std::string &signal_name,
                               int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name,
                               int domain_type,
                               int domain_idx,
                               double setting) override;
        protected:
            bool m_is_active;
            std::list<std::unique_ptr<IOGroup> > m_iogroup_list;
            std::vector<std::pair<IOGroup *, int> > m_active_signal;
            std::vector<std::pair<IOGroup *, int> > m_active_control;
    };
}

#endif
