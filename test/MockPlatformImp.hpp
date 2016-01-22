/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include "PlatformImp.hpp"

class MockPlatformImp : public geopm::PlatformImp {
    public:
        MOCK_CONST_METHOD0(package,
            int(void));
        MOCK_CONST_METHOD0(tile,
            int(void));
        MOCK_CONST_METHOD0(hw_cpu,
            int(void));
        MOCK_CONST_METHOD0(logical_cpu,
            int(void));
        MOCK_CONST_METHOD0(topology,
            geopm::PlatformTopology*(void));
        MOCK_METHOD0(initialize,
            void(void));
        MOCK_METHOD1(msr_path,
            void(int cpu_num));
        MOCK_METHOD1(model_supported,
            bool(int platform_id));
        MOCK_METHOD0(platform_name,
            std::string(void));
        MOCK_METHOD0(reset_msrs,
            void(void));
        MOCK_CONST_METHOD0(control_domain,
            int(void));
        MOCK_METHOD0(parse_hw_topology,
            void(void));
        MOCK_METHOD0(initialize_msrs,
            void());
};
