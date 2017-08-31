/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <stdlib.h>
#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "DeciderFactory.hpp"
#include "Decider.hpp"


class DeciderFactoryTest: public :: testing :: Test
{
    protected:
        void SetUp();
};

void DeciderFactoryTest::SetUp()
{
    setenv("GEOPM_PLUGIN_PATH", ".libs/", 1);
}

TEST_F(DeciderFactoryTest, decider_register)
{
    geopm::DeciderFactory factory;
    std::string dname("power_governing");
    geopm::IDecider *d = factory.decider(dname);
    ASSERT_FALSE(d == NULL);

    std::string ans = d->name();
    ASSERT_FALSE(ans.empty());
    EXPECT_EQ(0, ans.compare(dname));

    delete d;
}

TEST_F(DeciderFactoryTest, no_supported_decider)
{
    geopm::DeciderFactory factory;
    geopm::IDecider* d = NULL;
    int thrown = 0;
    try {
        d = factory.decider("doesntexist");
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    ASSERT_EQ(NULL, d);
    EXPECT_EQ(GEOPM_ERROR_DECIDER_UNSUPPORTED, thrown);
}
