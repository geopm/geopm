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

#ifndef PROCESSEPOCHIMP_HPP_INCLUDE
#define PROCESSEPOCHIMP_HPP_INCLUDE

#include "ProcessEpoch.hpp"

namespace geopm
{
    class ProcessEpochImp : public ProcessEpoch
    {
        public:
            ProcessEpochImp();
            virtual ~ProcessEpochImp() = default;

            void update(const ApplicationSampler::m_record_s &record) override;
            double last_epoch_runtime(void) const override;
            double last_epoch_runtime_network(void) const override;
            double last_epoch_runtime_ignore(void) const override;
            int epoch_count(void) const override;
        private:
            void reset_hint_map(std::map<uint64_t, double> &hint_map, double value);

            int m_epoch_count;
            double m_last_epoch_time;
            double m_last_runtime;

            uint64_t m_curr_hint;
            double m_last_hint_time;
            std::map<uint64_t, double> m_curr_hint_runtime;
            std::map<uint64_t, double> m_last_hint_runtime;

    };
}

#endif
