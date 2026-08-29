/*
 * Copyright (c) 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "geopm/IOGroup.hpp"
#include "geopm/Agg.hpp"

using geopm::IOGroup;

TEST(PMTIOGroupTest, plugin_is_registered)
{
    // Ensure the PMT IOGroup plugin is registered with the factory.
    auto names = IOGroup::iogroup_names();
    bool found = false;
    for (const auto &n : names) {
        if (n == "PMT") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(PMTIOGroupTest, signal_catalog_includes_virtuals_when_available)
{
    // If PMT IOGroup is present on this system, ensure expected virtual signals are listed.
    auto names = IOGroup::iogroup_names();
    if (std::find(names.begin(), names.end(), "PMT") == names.end()) {
        GTEST_SKIP() << "PMT IOGroup not available on this platform";
    }
    std::unique_ptr<IOGroup> pmt = IOGroup::make_unique("PMT");
    auto sigs = pmt->signal_names();
    // The IOGroup only advertises totals/rates if IDI signals were discovered; skip if not.
    bool have_any_idi = false;
    for (const auto &s : sigs) {
        if (s.rfind("PMT::IDI_PORT", 0) == 0) { have_any_idi = true; break; }
    }
    if (!have_any_idi) {
        GTEST_SKIP() << "No IDI signals discovered from PMT XML on this platform";
    }
    // Totals should be present
    EXPECT_NE(sigs.find("PMT::IDI_C2U_BW_TOTAL"), sigs.end());
    EXPECT_NE(sigs.find("PMT::IDI_U2C_BW_TOTAL"), sigs.end());
    // Rate totals should be present
    EXPECT_NE(sigs.find("PMT::IDI_C2U_BW_TOTAL_RATE"), sigs.end());
    EXPECT_NE(sigs.find("PMT::IDI_U2C_BW_TOTAL_RATE"), sigs.end());
}

TEST(PMTIOGroupTest, behavior_and_aggregation)
{
    auto names = IOGroup::iogroup_names();
    if (std::find(names.begin(), names.end(), "PMT") == names.end()) {
        GTEST_SKIP() << "PMT IOGroup not available on this platform";
    }
    std::unique_ptr<IOGroup> pmt = IOGroup::make_unique("PMT");
    auto sigs = pmt->signal_names();
    bool have_any_idi = false;
    for (const auto &s : sigs) {
        if (s.rfind("PMT::IDI_PORT", 0) == 0) { have_any_idi = true; break; }
    }
    if (!have_any_idi) {
        GTEST_SKIP() << "No IDI signals discovered from PMT XML on this platform";
    }
    // Check behaviors
    EXPECT_EQ(pmt->signal_behavior("PMT::IDI_C2U_BW_TOTAL"), IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE);
    EXPECT_EQ(pmt->signal_behavior("PMT::IDI_U2C_BW_TOTAL"), IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE);
    EXPECT_EQ(pmt->signal_behavior("PMT::IDI_C2U_BW_TOTAL_RATE"), IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE);
    EXPECT_EQ(pmt->signal_behavior("PMT::IDI_U2C_BW_TOTAL_RATE"), IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE);
    // Aggregation functions
    auto sum = geopm::Agg::sum;
    auto avg = geopm::Agg::average;
    EXPECT_TRUE(geopm::Agg::name(pmt->agg_function("PMT::IDI_C2U_BW_TOTAL")) == geopm::Agg::name(sum));
    EXPECT_TRUE(geopm::Agg::name(pmt->agg_function("PMT::IDI_U2C_BW_TOTAL")) == geopm::Agg::name(sum));
    EXPECT_TRUE(geopm::Agg::name(pmt->agg_function("PMT::IDI_C2U_BW_TOTAL_RATE")) == geopm::Agg::name(avg));
    EXPECT_TRUE(geopm::Agg::name(pmt->agg_function("PMT::IDI_U2C_BW_TOTAL_RATE")) == geopm::Agg::name(avg));
}
