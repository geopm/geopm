/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "gtest/gtest.h"
#include "TensorOneD.hpp"
#include "TensorOneDMatcher.hpp"

using geopm::TensorOneD;

::testing::Matcher<const TensorOneD&> TensorOneDEqualTo(const TensorOneD& expected) {
    return TensorOneDMatcher(expected);
}
