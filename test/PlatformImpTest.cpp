/*
 * Copyright (c) 2015, Intel Corporation
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
#include <string>
#include <fstream>
#include <system_error>

#include "gtest/gtest.h"
#include "PlatformImp.hpp"

#define NUM_CPU 16
#define NUM_TILE 4
#define NUM_PACKAGE 2

class TestPlatformImp : public geopm::PlatformImp
{
    public:
        TestPlatformImp();
        virtual ~TestPlatformImp();
        virtual bool model_supported(int platform_id);
        virtual std::string get_platform_name();
        virtual void set_msr_path(int cpu);
        virtual void initialize_msrs(void);
        virtual void reset_msrs(void);
};

TestPlatformImp::TestPlatformImp()
{
    m_hyperthreaded = false;
    m_num_cpu = NUM_CPU;
    m_num_tile = NUM_TILE;
    m_num_package = NUM_PACKAGE;

    for(off_t i = 0; (int)i < m_num_cpu; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        m_msr_offset_map.insert(std::pair<std::string, off_t>(name, i*64));
        open_msr(i);
    }
    //for negative tests
    m_cpu_file_descs.push_back(675);
}

TestPlatformImp::~TestPlatformImp()
{
    for(off_t i = 0; (int)i < m_num_cpu; i++) {
        close_msr(i);
    }
}

bool TestPlatformImp::model_supported(int platform_id)
{
    return (platform_id == 0x999);
}

std::string TestPlatformImp::get_platform_name()
{
    std::string name = "test_platform";
    return name;
}

void TestPlatformImp::initialize_msrs(void)
{
    return;
}

void TestPlatformImp::reset_msrs(void)
{
    return;
}

void TestPlatformImp::set_msr_path(int cpu)
{
    uint32_t lval = 0x0;
    uint32_t hval = 0xFFFFFFFF;
    int err;
    std::ofstream msrfile;

    err = snprintf(m_msr_path, 256, "/tmp/msrfile%d", cpu);
    ASSERT_TRUE(err >= 0);

    msrfile.open(m_msr_path, std::ios::out|std::ios::binary);
    ASSERT_TRUE(msrfile.is_open());

    for(uint64_t i = 0; i < NUM_CPU; i++) {
        lval = i;
        msrfile.write((const char*)&hval, sizeof(hval));
        msrfile.write((const char*)&lval, sizeof(lval));
    }

    msrfile.flush();
    msrfile.close();
    ASSERT_FALSE(msrfile.is_open());

    return;
}

class TestPlatformImp2 : public geopm::PlatformImp
{
    public:
        TestPlatformImp2()
        {
            ;
        }
        virtual ~TestPlatformImp2()
        {
            ;
        }
        virtual bool model_supported(int platform_id)
        {
            return true;
        }
        virtual void initialize_msrs(void)
        {
            return;
        }
        virtual void reset_msrs(void)
        {
            return;
        }
        virtual std::string get_platform_name();
    protected:
        FRIEND_TEST(PlatformImpTest, negative_open_msr);
        FRIEND_TEST(PlatformImpTest, parse_topology);
};

std::string TestPlatformImp2::get_platform_name()
{
    std::string name = "test_platform2";
    return name;
}
////////////////////////////////////////////////////////////////////

class PlatformImpTest: public :: testing :: Test
{
    public:
        PlatformImpTest();
        ~PlatformImpTest();
    protected:
        TestPlatformImp *m_platform;
};

PlatformImpTest::PlatformImpTest()
{
    m_platform = new TestPlatformImp();
}

PlatformImpTest::~PlatformImpTest()
{
    delete m_platform;
}

TEST_F(PlatformImpTest, platform_get_name)
{
    std::string pname = "test_platform";
    std::string ans;

    ans = m_platform->get_platform_name();
    ASSERT_FALSE(ans.empty());

    EXPECT_FALSE(ans.compare(pname));
}

TEST_F(PlatformImpTest, platform_get_package)
{
    int num = m_platform->get_num_package();

    EXPECT_TRUE(num == NUM_PACKAGE);
}

TEST_F(PlatformImpTest, platform_get_tile)
{
    int num = m_platform->get_num_tile();

    EXPECT_TRUE(num == NUM_TILE);
}

TEST_F(PlatformImpTest, platform_get_cpu)
{
    int num = m_platform->get_num_cpu();

    EXPECT_TRUE(num == NUM_CPU);
}

TEST_F(PlatformImpTest, platform_get_hyperthreaded)
{
    EXPECT_FALSE(m_platform->is_hyperthread_enabled());
}

TEST_F(PlatformImpTest, platform_get_offsets)
{
    for(int i = 0; i < 16; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        EXPECT_TRUE(m_platform->get_msr_offset(name) == (off_t)(i*64));
    }
}

TEST_F(PlatformImpTest, cpu_msr_read_write)
{
    uint64_t value;

    for(uint64_t i = 0; i < NUM_CPU; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        m_platform->write_msr(geopm::GEOPM_DOMAIN_CPU, i, name, i);
    }

    for(uint64_t i = 0; i < NUM_CPU; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        value = m_platform->read_msr(geopm::GEOPM_DOMAIN_CPU, i, name);
        EXPECT_TRUE((value == i));
    }
}

TEST_F(PlatformImpTest, tile_msr_read_write)
{
    uint64_t value;

    for(uint64_t i = 0; i < NUM_TILE; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        m_platform->write_msr(geopm::GEOPM_DOMAIN_TILE, i, name, i*3);
    }

    for(uint64_t i = 0; i < NUM_TILE; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        value = m_platform->read_msr(geopm::GEOPM_DOMAIN_TILE, i, name);
        EXPECT_TRUE((value == (i*3)));
    }
}

TEST_F(PlatformImpTest, package_msr_read_write)
{
    uint64_t value;

    for(uint64_t i = 0; i < NUM_PACKAGE; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        m_platform->write_msr(geopm::GEOPM_DOMAIN_PACKAGE, i, name, i*5);
    }

    for(uint64_t i = 0; i < NUM_PACKAGE; i++) {
        std::string name = "MSR_TEST_";
        name.append(std::to_string(i));
        value = m_platform->read_msr(geopm::GEOPM_DOMAIN_PACKAGE, i, name);
        EXPECT_TRUE((value == (i*5)));
    }
}

TEST_F(PlatformImpTest, negative_read_no_desc)
{
    int thrown = 0;
    std::string name = "MSR_TEST_0";

    try {
        (void) m_platform->read_msr(geopm::GEOPM_DOMAIN_CPU, (NUM_CPU+2), name);
    }
    catch(std::invalid_argument e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==1));
}

TEST_F(PlatformImpTest, negative_write_no_desc)
{
    uint64_t value = 0x5;
    int thrown = 0;
    std::string name = "MSR_TEST_0";

    try {
        m_platform->write_msr(geopm::GEOPM_DOMAIN_CPU, (NUM_CPU+2), name, value);
    }
    catch(std::invalid_argument e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==1));
}

TEST_F(PlatformImpTest, negative_read_bad_desc)
{
    int thrown = 0;
    std::string name = "MSR_TEST_0";

    try {
        (void) m_platform->read_msr(geopm::GEOPM_DOMAIN_CPU, NUM_CPU, name);
    }
    catch(std::runtime_error e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==1));
}

TEST_F(PlatformImpTest, negative_write_bad_desc)
{
    uint64_t value = 0x5;
    int thrown = 0;
    std::string name = "MSR_TEST_0";

    try {
        m_platform->write_msr(geopm::GEOPM_DOMAIN_CPU, NUM_CPU, name, value);
    }
    catch(std::runtime_error e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==1));
}

TEST_F(PlatformImpTest, negative_open_msr)
{
    int thrown = 0;
    TestPlatformImp2 p;

    try {
        p.open_msr(5000);
    }
    catch(std::system_error e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==1));
}

TEST_F(PlatformImpTest, parse_topology)
{
    int thrown = 0;
    TestPlatformImp2 p;

    try {
        p.parse_hw_topology();
    }
    catch(std::system_error e) {
        thrown = 1;
    }

    EXPECT_TRUE((thrown==0));

    EXPECT_TRUE((p.get_num_package() > 0));
    EXPECT_TRUE((p.get_num_cpu() > 0));
}

