/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef MOCKAPPLICATIONIO_HPP_INCLUDE
#define MOCKAPPLICATIONIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationIO.hpp"

class MockApplicationIO : public geopm::ApplicationIO
{
    public:
        MOCK_METHOD(void, connect, (), (override));
        MOCK_METHOD(bool, do_shutdown, (), (const, override));
        MOCK_METHOD(std::string, report_name, (), (const, override));
        MOCK_METHOD(std::string, profile_name, (), (const, override));
        MOCK_METHOD(std::set<std::string>, region_name_set, (), (const, override));
        MOCK_METHOD(double, total_region_runtime, (uint64_t region_id),
                    (const, override));
        MOCK_METHOD(double, total_region_runtime_mpi, (uint64_t region_id),
                    (const, override));
        MOCK_METHOD(double, total_epoch_runtime_ignore, (), (const, override));
        MOCK_METHOD(double, total_app_runtime_mpi, (), (const, override));
        MOCK_METHOD(double, total_app_runtime_ignore, (), (const, override));
        MOCK_METHOD(double, total_epoch_runtime, (), (const, override));
        MOCK_METHOD(double, total_epoch_runtime_network, (), (const, override));
        MOCK_METHOD(double, total_epoch_energy_pkg, (), (const, override));
        MOCK_METHOD(double, total_epoch_energy_dram, (), (const, override));
        MOCK_METHOD(int, total_epoch_count, (), (const, override));
        MOCK_METHOD(int, total_count, (uint64_t region_id), (const, override));
        MOCK_METHOD(void, update, (std::shared_ptr<geopm::Comm> comm), (override));
        MOCK_METHOD(std::list<geopm_region_info_s>, region_info, (), (const, override));
        MOCK_METHOD(void, clear_region_info, (), (override));
        MOCK_METHOD(void, controller_ready, (), (override));
        MOCK_METHOD(void, abort, (), (override));
};

#endif
