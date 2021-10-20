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

#ifndef MOCKPLATFORMIO_HPP_INCLUDE
#define MOCKPLATFORMIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "geopm/IOGroup.hpp"
#include "geopm/PlatformIO.hpp"

class MockPlatformIO : public geopm::PlatformIO
{
    public:
        MOCK_METHOD(void, register_iogroup,
                    (std::shared_ptr<geopm::IOGroup> iogroup), (override));
        MOCK_METHOD(std::set<std::string>, signal_names, (), (const, override));
        MOCK_METHOD(std::set<std::string>, control_names, (), (const, override));
        MOCK_METHOD(int, signal_domain_type, (const std::string &signal_name),
                    (const, override));
        MOCK_METHOD(int, control_domain_type, (const std::string &control_name),
                    (const, override));
        MOCK_METHOD(int, push_signal,
                    (const std::string &signal_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(int, push_control,
                    (const std::string &control_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(double, sample, (int signal_idx), (override));
        MOCK_METHOD(void, adjust, (int control_idx, double setting), (override));
        MOCK_METHOD(void, read_batch, (), (override));
        MOCK_METHOD(void, write_batch, (), (override));
        MOCK_METHOD(double, read_signal,
                    (const std::string &signal_name, int domain_type, int domain_idx),
                    (override));
        MOCK_METHOD(void, write_control,
                    (const std::string &control_name, int domain_type,
                     int domain_idx, double setting),
                    (override));
        MOCK_METHOD(void, save_control, (), (override));
        MOCK_METHOD(void, restore_control, (), (override));
        MOCK_METHOD(void, save_control, (const std::string &save_dir), (override));
        MOCK_METHOD(void, restore_control, (const std::string &save_dir), (override));
        MOCK_METHOD(std::function<double(const std::vector<double> &)>, agg_function,
                    (const std::string &signal_name), (const, override));
        MOCK_METHOD(std::function<std::string(double)>, format_function,
                    (const std::string &signal_name), (const, override));
        MOCK_METHOD(std::string, signal_description,
                    (const std::string &signal_name), (const, override));
        MOCK_METHOD(std::string, control_description,
                    (const std::string &control_name), (const, override));
        MOCK_METHOD(int, signal_behavior, (const std::string &signal_name),
                    (const, override));
        MOCK_METHOD(void, start_batch_server,
                    (int client_pid,
                     const std::vector<geopm_request_s> &signal_config,
                     const std::vector<geopm_request_s> &control_config,
                     int &server_pid,
                     std::string &server_key), (override));
        MOCK_METHOD(void, stop_batch_server, (int server_pid), (override));

};

#endif
