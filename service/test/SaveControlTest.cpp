/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SaveControl.hpp"

#include <iostream>

#include "gtest/gtest.h"

#include "geopm_test.hpp"
#include "geopm/Helper.hpp"
#include "MockIOGroup.hpp"
#include "MockPlatformTopo.hpp"

using geopm::SaveControl;
using geopm::SaveControlImp;

using testing::Return;
using testing::_;

class SaveControlTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        void check_settings(const std::vector<SaveControl::m_setting_s> &actual_settings);

        std::vector<SaveControl::m_setting_s> m_settings;
        std::string m_settings_json;
        std::string m_tmp_path;
        std::shared_ptr<MockIOGroup> m_mock_io_group;
        std::shared_ptr<MockPlatformTopo> m_mock_topo;
};

void SaveControlTest::SetUp(void)
{
    m_settings = {{"TEST::FREQUENCY", 2, 0, 1.0e9},
                  {"TEST::FREQUENCY", 2, 1, 2.0e9},
                  {"TEST::POWER", 1, 0, 300},
                  {"TEST::POWER", 1, 1, 310}};
    m_settings_json = "[{\"domain_idx\": 0, "
                        "\"domain_type\": 2, "
                        "\"name\": \"TEST::FREQUENCY\", "
                        "\"setting\": 1000000000}, "
                       "{\"domain_idx\": 1, "
                        "\"domain_type\": 2, "
                        "\"name\": \"TEST::FREQUENCY\", "
                        "\"setting\": 2000000000}, "
                       "{\"domain_idx\": 0, "
                        "\"domain_type\": 1, "
                        "\"name\": \"TEST::POWER\", "
                        "\"setting\": 300}, "
                       "{\"domain_idx\": 1, "
                        "\"domain_type\": 1, "
                        "\"name\": \"TEST::POWER\", "
                        "\"setting\": 310}]";
    m_tmp_path = "test_save_control_settings.json";
    m_mock_io_group = std::make_shared<MockIOGroup>();
    m_mock_topo = std::make_shared<MockPlatformTopo>();
}

void SaveControlTest::TearDown(void)
{
    (void)remove(m_tmp_path.c_str());
}

void SaveControlTest::check_settings(const std::vector<SaveControl::m_setting_s> &actual_settings)
{
    ASSERT_EQ(m_settings.size(), actual_settings.size());
    for (int idx = 0; idx != (int)m_settings.size(); ++idx) {
        ASSERT_EQ(m_settings[idx].name, actual_settings[idx].name);
        ASSERT_EQ(m_settings[idx].domain_type, actual_settings[idx].domain_type);
        ASSERT_EQ(m_settings[idx].domain_idx, actual_settings[idx].domain_idx);
        ASSERT_EQ(m_settings[idx].setting, actual_settings[idx].setting);
    }
}

TEST_F(SaveControlTest, static_json)
{
    std::string json_string = SaveControlImp::json(m_settings);
    ASSERT_EQ(m_settings_json, json_string);
}

TEST_F(SaveControlTest, static_settings)
{
    check_settings(SaveControlImp::settings(m_settings_json));
}

TEST_F(SaveControlTest, bad_json)
{
    std::string no_array_json = "{\"domain_idx\": 0, "
                                 "\"domain_type\": 2, "
                                 "\"name\": \"TEST::FREQUENCY\", "
                                 "\"setting\": 1000000000}";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(no_array_json),
                               GEOPM_ERROR_INVALID,
                               "Expected a JSON array");
    std::string no_object_json = "[[" + no_array_json + "]]";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(no_object_json),
                               GEOPM_ERROR_INVALID,
                               "Expected a JSON object");
    std::string wrong_field_json = "[{\"domain_idx\": 0, "
                                     "\"domain_kind\": 2, "
                                     "\"name\": \"TEST::FREQUENCY\", "
                                     "\"setting\": 1000000000}]";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(wrong_field_json),
                               GEOPM_ERROR_INVALID,
                               "Invalid settings object JSON, missing a required field: \"domain_type\"");

    std::string missing_field_json = "[{\"domain_idx\": 0, "
                                       "\"name\": \"TEST::FREQUENCY\", "
                                       "\"setting\": 1000000000}]";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(missing_field_json),
                               GEOPM_ERROR_INVALID,
                               "JSON object representing m_setting_s must have four fields");
    std::string extra_field_json = "[{\"domain_idx\": 0, "
                                     "\"domain_type\": 2, "
                                     "\"domain_kind\": 2, "
                                     "\"name\": \"TEST::FREQUENCY\", "
                                     "\"setting\": 1000000000}]";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(extra_field_json),
                               GEOPM_ERROR_INVALID,
                               "JSON object representing m_setting_s must have four fields");
    std::string invalid_json = "][";
    GEOPM_EXPECT_THROW_MESSAGE(SaveControlImp::settings(invalid_json),
                               GEOPM_ERROR_INVALID,
                               "unable to parse");
}

TEST_F(SaveControlTest, make_from_struct)
{
    auto save_ctl = SaveControl::make_unique(m_settings);
    check_settings(save_ctl->settings());
    ASSERT_EQ(m_settings_json, save_ctl->json());
}

TEST_F(SaveControlTest, make_from_string)
{
    auto save_ctl = SaveControl::make_unique(m_settings_json);
    check_settings(save_ctl->settings());
    ASSERT_EQ(m_settings_json, save_ctl->json());
}

TEST_F(SaveControlTest, make_from_io_group)
{
    m_settings = {{"TEST::FREQUENCY", 2, 0, 1.0e9},
                  {"TEST::FREQUENCY", 2, 1, 2.0e9},
                  {"TEST::POWER", 1, 0, 300},
                  {"TEST::POWER", 1, 1, 310}};
    EXPECT_CALL(*m_mock_io_group, name())
        .WillOnce(Return("TEST"));
    EXPECT_CALL(*m_mock_io_group, control_domain_type(_))
        .WillOnce(Return(2))
        .WillOnce(Return(1));
    EXPECT_CALL(*m_mock_io_group, control_names())
        .WillOnce(Return(std::set<std::string> {
            "FREQUENCY",
            "POWER",
            "TEST::FREQUENCY",
            "TEST::POWER"}));
    EXPECT_CALL(*m_mock_topo, num_domain(_))
        .WillOnce(Return(2))
        .WillOnce(Return(2));
    EXPECT_CALL(*m_mock_io_group, read_signal(_, _, _))
        .WillOnce(Return(1.0e9))
        .WillOnce(Return(2.0e9))
        .WillOnce(Return(300.0))
        .WillOnce(Return(310.0));
    SaveControlImp save_ctl(*m_mock_io_group, *m_mock_topo);
    check_settings(save_ctl.settings());
    ASSERT_EQ(m_settings_json, save_ctl.json());
    {
        EXPECT_CALL(*m_mock_io_group, write_control(_, _, _, _))
            .Times(4);
        save_ctl.restore(*m_mock_io_group);
    }
}

TEST_F(SaveControlTest, write_file)
{
    auto save_ctl = SaveControl::make_unique(m_settings);
    save_ctl->write_json(m_tmp_path);
    std::string actual_json_string = geopm::read_file(m_tmp_path);
    ASSERT_EQ(m_settings_json, actual_json_string);
}
