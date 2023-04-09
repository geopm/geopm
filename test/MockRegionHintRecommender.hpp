/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKREGIONHINTRECOMMENDER_HPP_INCLUDE
#define MOCKREGIONHINTRECOMMENDER_HPP_INCLUDE

#include "gmock/gmock.h"
#include "RegionHintRecommender.hpp"

namespace geopm {

class MockRegionHintRecommender : public RegionHintRecommender {
 public:
  MOCK_METHOD(double, recommend_frequency, ((std::map<std::string, double> region_class), double phi), (const, override));
};

}  // namespace geopm
#endif //MOCKREGIONHINTRECOMMENDER_HPP_INCLUDE
