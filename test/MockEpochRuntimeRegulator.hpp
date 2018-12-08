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

#ifndef MOCKEPOCHRUNTIMEREGULATOR_HPP_INCLUDE
#define MOCKEPOCHRUNTIMEREGULATOR_HPP_INCLUDE

#include "geopm_time.h"

#include "EpochRuntimeRegulator.hpp"
#include "RuntimeRegulator.hpp"


class MockEpochRuntimeRegulator : public geopm::IEpochRuntimeRegulator {
    public:
        MOCK_METHOD0(init_unmarked_region,
                     void());
        MOCK_METHOD2(epoch,
                     void(int rank, struct geopm_time_s epoch_time));
        MOCK_METHOD3(record_entry,
                     void(uint64_t region_id, int rank, struct geopm_time_s entry_time));
        MOCK_METHOD3(record_exit,
                     void(uint64_t region_id, int rank, struct geopm_time_s exit_time));
        MOCK_CONST_METHOD1(region_regulator,
                           const geopm::IRuntimeRegulator&(uint64_t region_id));
        MOCK_CONST_METHOD1(is_regulated,
                           bool(uint64_t region_id));
        MOCK_CONST_METHOD0(last_epoch_runtime,
                           std::vector<double>());
        MOCK_CONST_METHOD0(epoch_count,
                           std::vector<double>());
        MOCK_CONST_METHOD1(total_region_runtime,
                           double(uint64_t region_id));
        MOCK_CONST_METHOD1(total_region_runtime_mpi,
                           double(uint64_t region_id));
        MOCK_CONST_METHOD0(total_epoch_runtime,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_runtime_mpi,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_runtime_ignore,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_energy_pkg,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_energy_dram,
                           double(void));
        MOCK_CONST_METHOD0(total_app_runtime_mpi,
                           double(void));
        MOCK_CONST_METHOD1(total_count,
                           int(uint64_t region_id));
        MOCK_CONST_METHOD0(region_info,
                           std::list<geopm_region_info_s>(void));
        MOCK_METHOD0(clear_region_info,
                     void(void));
};

#endif
