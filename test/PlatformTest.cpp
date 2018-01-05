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

#include <iostream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "RAPLPlatform.hpp"
#include "MockPlatformImp.hpp"
#include "MockPlatformTopology.hpp"

using ::testing::Return;
using ::testing::Pointee;
using ::testing::_;
using ::testing::SetArgReferee;

class PlatformTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        geopm::Platform *platform;
        MockPlatformImp *platformimp;
        MockPlatformTopology *topo;
        std::vector<hwloc_obj_t> package;
        std::vector<hwloc_obj_t> package1_cpu;
        std::vector<hwloc_obj_t> package2_cpu;

};

void PlatformTest::SetUp()
{

    platform = new geopm::RAPLPlatform();
    platformimp = new MockPlatformImp();
    topo = new MockPlatformTopology();

    EXPECT_CALL(*platformimp, initialize());

    EXPECT_CALL(*platformimp, num_logical_cpu())
    .WillRepeatedly(Return(8));

    EXPECT_CALL(*platformimp, num_package())
    .WillRepeatedly(testing::Return(2));

    EXPECT_CALL(*platformimp, num_hw_cpu())
    .WillRepeatedly(testing::Return(8));

    EXPECT_CALL(*platformimp, num_package_signal())
    .WillRepeatedly(testing::Return(3));

    EXPECT_CALL(*platformimp, num_cpu_signal())
    .WillRepeatedly(testing::Return(5));

    platform->set_implementation((geopm::PlatformImp*)platformimp, true);
    hwloc_obj_t obj;

    for (int i = 0; i < 2; i++) {
        obj = (hwloc_obj_t)malloc(sizeof(struct hwloc_obj));
        obj->type = (hwloc_obj_type_t)(geopm::GEOPM_DOMAIN_PACKAGE);
        obj->os_index = i;
        obj->logical_index = i;
        package.push_back(obj);
    }
    for (int i = 0; i < 4; i++) {
        obj = (hwloc_obj_t)malloc(sizeof(struct hwloc_obj));
        obj->type = (hwloc_obj_type_t)(geopm::GEOPM_DOMAIN_CPU);
        obj->os_index = i;
        obj->logical_index = i;
        package1_cpu.push_back(obj);
    }
    for (int i = 4; i < 8; i++) {
        obj = (hwloc_obj_t)malloc(sizeof(struct hwloc_obj));
        obj->type = (hwloc_obj_type_t)(geopm::GEOPM_DOMAIN_CPU);
        obj->os_index = i;
        obj->logical_index = i;
        package2_cpu.push_back(obj);
    }
}

void PlatformTest::TearDown()
{
    delete topo;
    delete platformimp;
    delete platform;
}

MATCHER_P(Socket, x, "") {
    return (arg->logical_index == (unsigned)x);
}

TEST_F(PlatformTest, transform_init)
{
    std::vector<int> cpu_ranks({0, 0, 1, 1, 2, 2, 3, 3});
    std::vector<double> result(2 * GEOPM_NUM_TELEMETRY_TYPE);
    std::vector<double> expect({1, 1, 1, 4, 4, 4, 4, 4, 2, 2, 1, 1, 1, 4, 4, 4, 4, 4, 2, 2});

    EXPECT_EQ(expect.size(), result.size());

    EXPECT_CALL(*platformimp, topology())
    .WillOnce(testing::Return(topo));

    EXPECT_CALL(*platformimp, power_control_domain())
    .WillOnce(Return(geopm::GEOPM_DOMAIN_PACKAGE));

    EXPECT_CALL(*topo, num_domain(_))
    .WillOnce(Return(2));

    EXPECT_CALL(*topo, domain_by_type(_,_))
    .WillRepeatedly(SetArgReferee<1>(package));

    EXPECT_CALL(*topo, children_by_type(_,Socket(0),_))
    .WillRepeatedly(SetArgReferee<2>(package1_cpu));

    EXPECT_CALL(*topo, children_by_type(_,Socket(1),_))
    .WillRepeatedly(SetArgReferee<2>(package2_cpu));

    platform->init_transform(cpu_ranks);

#if 0 // This needs to be changed to match new code
    const std::vector<double> *transform = platform->signal_domain_transform();

    for (unsigned i = 0; i < 2 * GEOPM_NUM_TELEMETRY_TYPE; ++i) {
        result[i] = 0.0;
        for (unsigned j = 0; j < (platform->capacity() + (4 * 2)); ++j) {
            result[i] += (*transform)[i * (platform->capacity() + (4 * 2)) +j];
        }
    }

    for (unsigned i = 0; i < 2 * GEOPM_NUM_TELEMETRY_TYPE; ++i) {
        EXPECT_DOUBLE_EQ(expect[i], result[i]);
    }
#endif
}
