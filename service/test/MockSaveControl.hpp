/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSAVECONTROL_HPP_INCLUDE
#define MOCKSAVECONTROL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SaveControl.hpp"
#include "geopm/IOGroup.hpp"


class MockSaveControl : public geopm::SaveControl
{
    public:
        MOCK_METHOD(std::string, json, (), (const, override));
        MOCK_METHOD(std::vector<m_setting_s>, settings, (), (const, override));
        MOCK_METHOD(void, write_json, (const std::string &save_path), (const, override));
        MOCK_METHOD(void, restore, (geopm::IOGroup &io_group), (const, override));
        MOCK_METHOD(std::set<std::string>, unsaved_controls, (const std::set<std::string> &all_controls), (const, override));
};

#endif
