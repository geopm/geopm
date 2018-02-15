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
#ifndef FREQIOGROUP_HPP_INCLUDE
#define FREQIOGROUP_HPP_INCLUDE

#include "IOGroup.hpp"

namespace geopm
{
    class FreqIOGroup : public IOGroup
    {
        public:
            FreqIOGroup();
            virtual ~FreqIOGroup();
            bool is_valid_signal(const std::string &signal_name) override;
            bool is_valid_control(const std::string &control_name) override;
            int signal_domain_type(const std::string &signal_name) override;
            int control_domain_type(const std::string &control_name) override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        protected:
            enum m_cpu_bound_e {
                M_CPU_BOUND_INVALID = -1,
                M_CPU_BOUND_MIN = 0,
                M_CPU_BOUND_STICKER,
                M_CPU_BOUND_MAX,
                M_CPU_BOUND_NUM
            };
            double m_cpu_bound[M_CPU_BOUND_NUM];
    };
}

#endif
