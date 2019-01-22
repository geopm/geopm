/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <numeric>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TreeComm.hpp"
#include "TreeCommLevel.hpp"
#include "MockComm.hpp"
#include "MockTreeCommLevel.hpp"
#include "geopm_test.hpp"
#include "config.h"

using geopm::ITreeCommLevel;
using geopm::TreeComm;
using testing::_;
using testing::Return;
using testing::DoAll;
using testing::SetArgReferee;

class TreeCommTest : public ::testing::Test
{
    protected:
        void SetUp();
        void root_setup();
        void nonroot_setup();

        std::shared_ptr<MockComm> m_mock_comm;
        std::vector<int> m_fan_out;
        std::vector<MockTreeCommLevel *> m_level_ptr;
        std::unique_ptr<TreeComm> m_tree_comm;
        int m_num_send_up = 3;
        int m_num_send_down = 2;
};

void TreeCommTest::SetUp()
{
    m_mock_comm = std::make_shared<MockComm>();
}

void TreeCommTest::root_setup()
{
    m_fan_out = {2, 3, 4, 5};
    std::vector<std::unique_ptr<ITreeCommLevel> > temp;
    for (size_t lvl = 0; lvl < m_fan_out.size(); ++lvl) {
        m_level_ptr.push_back(new MockTreeCommLevel);
        temp.emplace_back(m_level_ptr[lvl]);
    }

    EXPECT_CALL(*m_mock_comm, barrier());
    EXPECT_CALL(*m_mock_comm, num_rank()).WillOnce(Return(120));
    m_tree_comm.reset(new TreeComm(m_mock_comm, m_fan_out, m_fan_out.size(),
                                   m_num_send_down, m_num_send_up, std::move(temp)));
}

void TreeCommTest::nonroot_setup()
{
    m_fan_out = {2, 3, 4, 5};
    std::vector<std::unique_ptr<ITreeCommLevel> > temp;
    for (size_t lvl = 0; lvl < m_fan_out.size(); ++lvl) {
        m_level_ptr.push_back(new MockTreeCommLevel);
        temp.emplace_back(m_level_ptr[lvl]);
    }

    EXPECT_CALL(*m_mock_comm, barrier());
    EXPECT_CALL(*m_mock_comm, num_rank()).WillOnce(Return(120));
    m_tree_comm.reset(new TreeComm(m_mock_comm, m_fan_out, m_fan_out.size() - 1,
                                   m_num_send_down, m_num_send_up, std::move(temp)));
}

TEST_F(TreeCommTest, geometry)
{
    // tree comm controlling up to root
    root_setup();

    EXPECT_EQ(4, m_tree_comm->num_level_controlled());
    EXPECT_EQ(4, m_tree_comm->root_level());
    EXPECT_EQ(4, m_tree_comm->max_level());
    for (size_t level = 0; level < m_fan_out.size(); ++level) {
        int rank = 5 - level;
        EXPECT_CALL(*(m_level_ptr[level]), level_rank()).WillOnce(Return(rank));
        EXPECT_EQ(rank, m_tree_comm->level_rank(level));
        EXPECT_EQ(m_fan_out[m_fan_out.size() - level - 1], m_tree_comm->level_size(level));
    }
    // errors
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->level_rank(-1), GEOPM_ERROR_LEVEL_RANGE,
                               "level_rank");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->level_rank(10), GEOPM_ERROR_LEVEL_RANGE,
                               "level_rank");

    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->level_size(-1), GEOPM_ERROR_LEVEL_RANGE,
                               "level_size");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->level_size(10), GEOPM_ERROR_LEVEL_RANGE,
                               "level_size");
}

TEST_F(TreeCommTest, geometry_nonroot)
{
    nonroot_setup();

    EXPECT_EQ(3, m_tree_comm->num_level_controlled());
    EXPECT_EQ(4, m_tree_comm->root_level());
    EXPECT_EQ(4, m_tree_comm->max_level());
    for (size_t level = 0; level < m_fan_out.size(); ++level) {
        int rank = 5 - level;
        EXPECT_CALL(*(m_level_ptr[level]), level_rank()).WillOnce(Return(rank));
        EXPECT_EQ(rank, m_tree_comm->level_rank(level));
        EXPECT_EQ(m_fan_out[m_fan_out.size() - level - 1], m_tree_comm->level_size(level));
    }
}

TEST_F(TreeCommTest, send_receive)
{
    root_setup();

    std::vector<double> sample {10.0, 11.0, 12.0};
    std::vector<std::vector<double> > expected_sample {sample, sample};
    std::vector<std::vector<double> > recv_sample(2, std::vector<double>(3));
    std::vector<std::vector<double> > policy {{9.0}, {8.0}};
    std::vector<double> recv_policy(1);

    for (size_t level = 0; level < m_level_ptr.size(); ++level) {
        EXPECT_CALL(*(m_level_ptr[level]), send_up(sample));
        m_tree_comm->send_up(level, sample);

        EXPECT_CALL(*(m_level_ptr[level]), send_down(policy));
        m_tree_comm->send_down(level, policy);

        if (level) {
            EXPECT_CALL(*(m_level_ptr[level]), receive_up(_)).WillOnce(
                DoAll(SetArgReferee<0>(expected_sample), Return(true)));
            EXPECT_TRUE(m_tree_comm->receive_up(level, recv_sample));
            EXPECT_EQ(expected_sample, recv_sample);
        }

        EXPECT_CALL(*(m_level_ptr[level]), receive_down(_)).WillOnce(
            DoAll(SetArgReferee<0>(policy[0]), Return(true)));
        EXPECT_TRUE(m_tree_comm->receive_down(level, recv_policy));
        EXPECT_EQ(policy[0], recv_policy);
    }

    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->send_up(-1, sample),
                               GEOPM_ERROR_LEVEL_RANGE, "send_up");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->send_down(-1, policy),
                               GEOPM_ERROR_LEVEL_RANGE, "send_down");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->receive_up(-1, recv_sample),
                               GEOPM_ERROR_LEVEL_RANGE, "receive_up");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->receive_down(-1, recv_policy),
                               GEOPM_ERROR_LEVEL_RANGE, "receive_down");

    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->send_up(10, sample),
                               GEOPM_ERROR_LEVEL_RANGE, "send_up");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->send_down(10, policy),
                               GEOPM_ERROR_LEVEL_RANGE, "send_down");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->receive_up(10, recv_sample),
                               GEOPM_ERROR_LEVEL_RANGE, "receive_up");
    GEOPM_EXPECT_THROW_MESSAGE(m_tree_comm->receive_down(10, recv_policy),
                               GEOPM_ERROR_LEVEL_RANGE, "receive_down");
}

TEST_F(TreeCommTest, overhead_send)
{
    root_setup();

    std::vector<size_t> overhead{67, 78, 89, 90};
    size_t expected_overhead = std::accumulate(overhead.begin(), overhead.end(), 0);
    for (size_t level = 0; level < m_level_ptr.size(); ++level) {
        EXPECT_CALL(*(m_level_ptr[level]), overhead_send())
            .WillOnce(Return(overhead[level]));
    }

    EXPECT_EQ(expected_overhead, m_tree_comm->overhead_send());
}
