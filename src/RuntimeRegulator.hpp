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

#ifndef RUNTIMEREGULATOR_HPP_INCLUDE
#define RUNTIMEREGULATOR_HPP_INCLUDE

#include <vector>
#include <string>

#include "geopm_time.h"
#include "geopm_message.h"

namespace geopm
{
    class IRuntimeRegulator
    {
        public:
            IRuntimeRegulator() = default;
            virtual ~IRuntimeRegulator() = default;
            virtual void record_entry(int rank, struct geopm_time_s entry_time) = 0;
            virtual void record_exit(int rank, struct geopm_time_s exit_time) = 0;
            virtual void insert_runtime_signal(std::vector<struct geopm_telemetry_message_s> &telemetry) = 0;
            virtual std::vector<double> runtimes(void) const = 0;
    };

    class RuntimeRegulator : public IRuntimeRegulator
    {
        public:
            RuntimeRegulator() = delete;
            RuntimeRegulator(int max_rank_count);
            virtual ~RuntimeRegulator() override;
            void record_entry(int rank, struct geopm_time_s entry_time) override;
            void record_exit(int rank, struct geopm_time_s exit_time) override;
            void insert_runtime_signal(std::vector<struct geopm_telemetry_message_s> &telemetry) override;
            std::vector<double> runtimes(void) const override;

        protected:
            void update_average(void);
            const struct geopm_time_s M_TIME_ZERO;
            enum m_num_rank_signal_e {
                M_NUM_RANK_SIGNAL = 2,
            };
            int m_max_rank_count;
            double m_last_avg;
            // per rank vector of last entry and recorded runtime pairs
            std::vector<std::pair<struct geopm_time_s, double> > m_runtimes;
    };

    class MPIRuntimeRegulator : public RuntimeRegulator
    {
        public:
            MPIRuntimeRegulator() = delete;
            MPIRuntimeRegulator(int max_rank_count);
            virtual ~MPIRuntimeRegulator() = default;
        protected:
            void update_average(void);
    };
}

#endif
