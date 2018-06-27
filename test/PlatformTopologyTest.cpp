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

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#include <unistd.h>
#include <hwloc.h>

#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "PlatformTopology.hpp"

class PlatformTopologyTest: public :: testing :: Test
{
    protected:
        geopm::PlatformTopology m_topo;
};

TEST_F(PlatformTopologyTest, cpu_count)
{
#ifdef _SC_NPROCESSORS_ONLN
    int expect = sysconf(_SC_NPROCESSORS_ONLN);
#else
    int expect = 1;
    size_t len = sizeof(expect);
    sysctl((int[2]) {CTL_HW, HW_NCPU}, 2, &expect, &len, NULL, 0);
#endif
    int actual = m_topo.num_domain(geopm::GEOPM_DOMAIN_CPU);

    EXPECT_EQ(expect, actual);
}

TEST_F(PlatformTopologyTest, negative_num_domain)
{
    int thrown = 0;
    int val = 0;

    try {
        val = m_topo.num_domain(HWLOC_OBJ_TYPE_MAX);
    }
    catch (geopm::Exception& e) {
        thrown = e.err_value();
    }

    EXPECT_EQ(val, 0);
    EXPECT_EQ(thrown, GEOPM_ERROR_INVALID);
}

