/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"

#include "geopm_hint.h"
#include "geopm_test.hpp"

TEST(GEOPMHintTest, check_hint)
{
    uint64_t hint = GEOPM_SENTINEL_REGION_HINT;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "hint out of range");
    hint = GEOPM_SENTINEL_REGION_HINT + 1;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "hint out of range");
}
