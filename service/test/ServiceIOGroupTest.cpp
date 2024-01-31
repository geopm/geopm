/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <vector>
#include <memory>
#include <map>

#include "geopm_topo.h"

#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "ServiceIOGroup.hpp"
#include "MockServiceProxy.hpp"
#include "MockPlatformTopo.hpp"
#include "MockBatchClient.hpp"
#include "geopm_test.hpp"

using geopm::signal_info_s;
using geopm::control_info_s;
using geopm::ServiceIOGroup;
using geopm::Exception;
using testing::AtLeast;
using testing::Return;
using testing::_;
using testing::SetArgReferee;
using testing::DoAll;

class ServiceIOGroupTest : public :: testing:: Test
{
    protected:
        void SetUp();

        std::unique_ptr<ServiceIOGroup> m_serviceio_group;
        std::shared_ptr<MockServiceProxy> m_proxy;
        std::shared_ptr<MockPlatformTopo> m_topo;
        std::shared_ptr<MockBatchClient> m_batch_client;

        int m_num_package = 2;
        int m_num_core = 4;
        int m_num_cpu = 16;

        std::vector<std::string> m_expected_signals;
        std::vector<std::string> m_expected_controls;
        std::map<std::string, signal_info_s> m_signal_info;
        std::map<std::string, control_info_s> m_control_info;
};

void ServiceIOGroupTest::SetUp()
{
    m_topo = make_topo(m_num_package, m_num_core, m_num_cpu);
    m_proxy = std::make_shared<MockServiceProxy>();
    m_batch_client = std::make_shared<MockBatchClient>();
    EXPECT_CALL(*m_topo, num_domain(_)).Times(AtLeast(0));

    m_expected_signals = {"signal1", "signal2"};
    m_expected_controls = {"control1", "control2"};

    // signal_info_s: (str)name, (str)desc, (int)domain, (int)agg, (int)string_format, (int)behavior
    m_signal_info =
        {{m_expected_signals[0],
         {m_expected_signals[0], "1 Signal", 0, 0, 0, 0}},
         {m_expected_signals[1],
         {m_expected_signals[1], "2 Signal", 1, 1, 1, 1}},
        };

    // control_info_s: (str)name, (str)desc, (int)domain
    m_control_info =
        {{m_expected_controls[0],
         {m_expected_controls[0], "1 Control", 0}},
         {m_expected_controls[1],
         {m_expected_controls[1], "2 Control", 1}},
        };

    EXPECT_CALL(*m_proxy, platform_get_user_access(_, _))
        .WillRepeatedly([this]
                        (std::vector<std::string> &signals,
                         std::vector<std::string> &controls) {
            signals = m_expected_signals;
            controls = m_expected_controls;
        });

    std::vector<signal_info_s> expected_signal_info = {m_signal_info[m_expected_signals[0]],
                                                       m_signal_info[m_expected_signals[1]]};
    std::vector<control_info_s> expected_control_info = {m_control_info[m_expected_controls[0]],
                                                         m_control_info[m_expected_controls[1]]};

    EXPECT_CALL(*m_proxy, platform_get_signal_info(m_expected_signals))
        .WillOnce(Return(expected_signal_info));
    EXPECT_CALL(*m_proxy, platform_get_control_info(m_expected_controls))
        .WillOnce(Return(expected_control_info));

    EXPECT_CALL(*m_proxy, platform_open_session());
    EXPECT_CALL(*m_proxy, platform_close_session());

    m_serviceio_group = geopm::make_unique<ServiceIOGroup>(*m_topo,
                                                           m_proxy,
                                                           m_batch_client);
}

TEST_F(ServiceIOGroupTest, signal_control_info)
{
    auto signal_names = m_serviceio_group->signal_names();
    auto control_names = m_serviceio_group->control_names();

    for (auto sig : m_expected_signals) {
        EXPECT_TRUE(m_serviceio_group->is_valid_signal(sig));
        EXPECT_TRUE(signal_names.find(sig) != signal_names.end());
        EXPECT_TRUE(signal_names.find("SERVICE::" + sig) != signal_names.end());
        EXPECT_EQ(m_signal_info[sig].description, m_serviceio_group->signal_description(sig));
    }
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->signal_description("BAD SIGNAL"),
                               GEOPM_ERROR_INVALID,
                               "BAD SIGNAL not valid for ServiceIOGroup");

    for (auto con : m_expected_controls) {
        EXPECT_TRUE(m_serviceio_group->is_valid_control(con));
        EXPECT_TRUE(control_names.find(con) != control_names.end());
        EXPECT_TRUE(control_names.find("SERVICE::" + con) != control_names.end());
        EXPECT_EQ(m_control_info[con].description, m_serviceio_group->control_description(con));
    }
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->control_description("BAD CONTROL"),
                               GEOPM_ERROR_INVALID,
                               "BAD CONTROL not valid for ServiceIOGroup");
}

TEST_F(ServiceIOGroupTest, domain_type)
{
    for (int ii = 0; ii < (int)m_expected_signals.size(); ++ii) {
        EXPECT_EQ(ii, m_serviceio_group->signal_domain_type(m_expected_signals.at(ii)));
        EXPECT_EQ(ii, m_serviceio_group->signal_domain_type("SERVICE::" + m_expected_signals.at(ii)));
    }
    for (int ii = 0; ii < (int)m_expected_controls.size(); ++ii) {
        EXPECT_EQ(ii, m_serviceio_group->control_domain_type(m_expected_controls.at(ii)));
        EXPECT_EQ(ii, m_serviceio_group->control_domain_type("SERVICE::" + m_expected_controls.at(ii)));
    }
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_serviceio_group->signal_domain_type("BAD SIGNAL"));
    EXPECT_EQ(GEOPM_DOMAIN_INVALID, m_serviceio_group->control_domain_type("BAD CONTROL"));
}

TEST_F(ServiceIOGroupTest, read_signal_behavior)
{
    for (unsigned ii = 0; ii < m_expected_signals.size(); ++ii) {
        EXPECT_CALL(*m_proxy, platform_read_signal(m_expected_signals.at(ii), ii, ii))
            .WillOnce(Return(42))
            .WillOnce(Return(7));
        EXPECT_EQ(42, m_serviceio_group->read_signal(m_expected_signals.at(ii), ii, ii));
        EXPECT_EQ(7, m_serviceio_group->read_signal("SERVICE::" + m_expected_signals.at(ii), ii, ii));
        EXPECT_EQ((int)ii, m_serviceio_group->signal_behavior(m_expected_signals.at(ii)));
    }
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->signal_behavior("BAD SIGNAL"),
                               GEOPM_ERROR_INVALID,
                               "BAD SIGNAL not valid for ServiceIOGroup");
}

TEST_F(ServiceIOGroupTest, read_signal_exception)
{
    // !is_valid_signal(signal_name)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->read_signal("NUM_VACUUM_TUBES", 4, 0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::read_signal(): signal name \"NUM_VACUUM_TUBES\" not found");

    // domain_type != signal_domain_type(signal_name)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->read_signal(m_expected_signals[0], 80, 0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::read_signal(): domain_type requested does not match the domain of the signal (");

    // domain_idx < 0
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->read_signal(m_expected_signals[0], 0, -8),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::read_signal(): domain_idx out of range");

    // domain_idx >= m_platform_topo.num_domain(domain_type)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->read_signal(m_expected_signals[0], 0, 80),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::read_signal(): domain_idx out of range");
}

TEST_F(ServiceIOGroupTest, write_control)
{
    for (unsigned ii = 0; ii < m_expected_controls.size(); ++ii) {
        EXPECT_CALL(*m_proxy, platform_write_control(m_expected_controls.at(ii), ii, ii, 42));
        EXPECT_NO_THROW(m_serviceio_group->write_control(m_expected_controls.at(ii), ii, ii, 42));
        EXPECT_CALL(*m_proxy, platform_write_control(m_expected_controls.at(ii), ii, ii, 7));
        EXPECT_NO_THROW(m_serviceio_group->write_control("SERVICE::" + m_expected_controls.at(ii),
                                                         ii, ii, 7));
    }
}

TEST_F(ServiceIOGroupTest, write_control_exception)
{
    // !is_valid_control(control_name)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->write_control("NUM_VACUUM_TUBES", 4, 0, 7.0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::write_control(): control name \"NUM_VACUUM_TUBES\" not found");

    // domain_type != control_domain_type(control_name)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->write_control(m_expected_controls[0], 80, 0, 7.0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::write_control(): domain_type does not match the domain of the control.");

    // domain_idx < 0
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->write_control(m_expected_controls[0], 0, -8, 7.0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::write_control(): domain_idx out of range");

    // domain_idx >= m_platform_topo.num_domain(domain_type)
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->write_control(m_expected_controls[0], 0, 80, 7.0),
                               GEOPM_ERROR_INVALID,
                               "ServiceIOGroup::write_control(): domain_idx out of range");
}

TEST_F(ServiceIOGroupTest, valid_signal_aggregation)
{
    std::function<double(const std::vector<double> &)> func;
    func = m_serviceio_group->agg_function("signal1");
    EXPECT_TRUE(is_agg_sum(func));
    func = m_serviceio_group->agg_function("signal2");
    EXPECT_TRUE(is_agg_average(func));
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->agg_function("BAD SIGNAL"),
                               GEOPM_ERROR_INVALID,
                               "BAD SIGNAL not valid for ServiceIOGroup");
}

TEST_F(ServiceIOGroupTest, valid_format_function)
{
    std::function<std::string(double)> func;
    func = m_serviceio_group->format_function("signal1");
    EXPECT_TRUE(is_format_double(func));
    func = m_serviceio_group->format_function("signal2");
    EXPECT_TRUE(is_format_integer(func));
    GEOPM_EXPECT_THROW_MESSAGE(m_serviceio_group->format_function("BAD SIGNAL"),
                               GEOPM_ERROR_INVALID,
                               "BAD SIGNAL not valid for ServiceIOGroup");
}

TEST_F(ServiceIOGroupTest, push_signal)
{
    EXPECT_CALL(*m_proxy, platform_start_batch(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(1234),
                        SetArgReferee<3>("1234")));
    std::vector<double> expected_result = {4.321012};
    EXPECT_CALL(*m_batch_client, read_batch())
        .WillOnce(Return(expected_result));
    int signal_handle = m_serviceio_group->push_signal("signal1", GEOPM_DOMAIN_BOARD, 0);
    m_serviceio_group->read_batch();
    double actual_result = m_serviceio_group->sample(signal_handle);
    EXPECT_EQ(expected_result[0], actual_result);
    EXPECT_CALL(*m_batch_client, stop_batch())
        .Times(1);
    m_batch_client.reset();
}

TEST_F(ServiceIOGroupTest, push_control)
{
    std::vector<double> expected_setting = {4.321012};
    EXPECT_CALL(*m_proxy, platform_start_batch(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(1234),
                        SetArgReferee<3>("1234")));
    EXPECT_CALL(*m_batch_client, write_batch(_))
        .Times(1);
    int control_handle = m_serviceio_group->push_control("control1", GEOPM_DOMAIN_BOARD, 0);
    m_serviceio_group->adjust(control_handle, expected_setting[0]);
    m_serviceio_group->write_batch();
    EXPECT_CALL(*m_batch_client, stop_batch())
        .Times(1);
    m_batch_client.reset();
}

TEST_F(ServiceIOGroupTest, read_batch)
{
    // For now this should be a noop
    m_serviceio_group->read_batch();
}

TEST_F(ServiceIOGroupTest, write_batch)
{
    // For now this should be a noop
    m_serviceio_group->write_batch();
}

TEST_F(ServiceIOGroupTest, save_control)
{
    /// These are noops, make sure mock objects are not called into
    /// We want it to fail if it tries to open the non-existent file path.
    m_serviceio_group->save_control();
    m_serviceio_group->save_control("/bad/file/path");
}

TEST_F(ServiceIOGroupTest, restore_control)
{
    EXPECT_CALL(*m_proxy, platform_restore_control());
    m_serviceio_group->restore_control();
    /// This is a noop, make sure mock objects are not called into
    /// We want it to fail if it tries to open the non-existent file path.
    m_serviceio_group->restore_control("/bad/file/path");
}
