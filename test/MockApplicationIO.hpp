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

#ifndef MOCKAPPLICATIONIO_HPP_INCLUDE
#define MOCKAPPLICATIONIO_HPP_INCLUDE

#include "ApplicationIO.hpp"

class MockApplicationIO : public geopm::IApplicationIO
{
    public:
        MOCK_METHOD0(connect,
                     void(void));
        MOCK_CONST_METHOD0(do_shutdown,
                           bool(void));
        MOCK_CONST_METHOD0(report_name,
                           std::string(void));
        MOCK_CONST_METHOD0(profile_name,
                           std::string(void));
        MOCK_CONST_METHOD0(region_name_set,
                           std::set<std::string>(void));
        MOCK_CONST_METHOD1(total_region_runtime,
                           double(uint64_t region_id));
        MOCK_CONST_METHOD1(total_region_runtime_mpi,
                           double(uint64_t region_id));
        MOCK_CONST_METHOD0(total_app_runtime,
                           double(void));
        MOCK_CONST_METHOD1(total_app_energy_pkg,
                           double(int pkg_idx));
        MOCK_CONST_METHOD0(total_app_energy_dram,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_runtime_ignore,
                           double(void));
        MOCK_CONST_METHOD0(total_app_runtime_mpi,
                           double(void));
        MOCK_CONST_METHOD0(total_app_runtime_ignore,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_runtime,
                           double(void));
        MOCK_CONST_METHOD0(total_epoch_runtime_mpi,
                           double(void));
        MOCK_CONST_METHOD1(total_epoch_energy_pkg,
                           double(int pkg_idx));
        MOCK_CONST_METHOD0(total_epoch_energy_dram,
                           double(void));
        MOCK_CONST_METHOD1(total_count,
                           int(uint64_t region_id));
        MOCK_METHOD1(update,
                     void(std::shared_ptr<geopm::Comm> comm));
        MOCK_METHOD0(profile_io_group,
                     std::shared_ptr<geopm::IOGroup>(void));
        MOCK_CONST_METHOD0(region_info,
                           std::list<geopm_region_info_s>(void));
        MOCK_METHOD0(clear_region_info,
                     void(void));
        MOCK_METHOD0(controller_ready,
                     void(void));
        MOCK_METHOD0(abort,
                     void(void));
};

#endif
