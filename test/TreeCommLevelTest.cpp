/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include <numeric>
#include <cmath>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TreeCommLevel.hpp"
#include "MockComm.hpp"
#include "geopm_test.hpp"

using geopm::TreeCommLevel;
using testing::Return;
using testing::Invoke;
using testing::SetArgPointee;
using testing::_;

class TreeCommLevelTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        int m_num_up = 3;
        int m_num_down = 2;
        int m_num_rank = 4;
        std::shared_ptr<MockComm> m_comm_0;
        std::shared_ptr<MockComm> m_comm_1;
        std::shared_ptr<TreeCommLevel> m_level_rank_0;
        std::shared_ptr<TreeCommLevel> m_level_rank_1;
        double *m_policy_mem_0;
        double *m_policy_mem_1;
        double *m_sample_mem_0;
        double *m_sample_mem_1;
        int *m_sample_window[2] = {nullptr, nullptr};
        int *m_policy_window[2] = {nullptr, nullptr};
};


void TreeCommLevelTest::SetUp()
{
    m_comm_0 = std::make_shared<MockComm>();
    m_comm_1 = std::make_shared<MockComm>();

    EXPECT_CALL(*m_comm_0, num_rank()).WillOnce(Return(m_num_rank));
    EXPECT_CALL(*m_comm_1, num_rank()).WillOnce(Return(m_num_rank));
    EXPECT_CALL(*m_comm_0, rank()).WillOnce(Return(0));
    EXPECT_CALL(*m_comm_1, rank()).WillOnce(Return(1));
    // set up memory to be returned by alloc_mem
    size_t policy_size = sizeof(double) * (m_num_down + 1);
    size_t sample_size = sizeof(double) * m_num_rank * (m_num_up + 1);
    m_policy_mem_0 = (double*)malloc(policy_size);
    m_policy_mem_1 = (double*)malloc(policy_size);
    m_sample_mem_0 = (double*)malloc(sample_size);
    m_sample_mem_1 = (double*)malloc(sample_size);
    EXPECT_CALL(*m_comm_0, alloc_mem(sample_size, _))
        .WillOnce(SetArgPointee<1>(m_sample_mem_0));
    EXPECT_CALL(*m_comm_1, alloc_mem(sample_size, _))
        .WillOnce(SetArgPointee<1>(m_sample_mem_1));
    EXPECT_CALL(*m_comm_0, alloc_mem(policy_size, _))
        .WillOnce(SetArgPointee<1>(m_policy_mem_0));
    EXPECT_CALL(*m_comm_1, alloc_mem(policy_size, _))
        .WillOnce(SetArgPointee<1>(m_policy_mem_1));

    m_sample_window[0] = new int(77);
    m_sample_window[1] = new int(78);
    m_policy_window[0] = new int(87);
    m_policy_window[1] = new int(88);

    // rank 0 only sets up sample window
    EXPECT_CALL(*m_comm_0, window_create(sample_size, _)).WillOnce(Return((size_t)m_sample_window[0]));
    EXPECT_CALL(*m_comm_0, window_create(0, NULL)).WillOnce(Return((size_t)m_policy_window[0])); // policy window

    // rank 1 only sets up policy window
    EXPECT_CALL(*m_comm_1, window_create(0, NULL)).WillOnce(Return((size_t)m_sample_window[1])); // sample window
    EXPECT_CALL(*m_comm_1, window_create(policy_size, _)).WillOnce(Return((size_t)m_policy_window[1]));

    m_level_rank_0 = std::make_shared<TreeCommLevel>(m_comm_0, m_num_up, m_num_down);
    m_level_rank_1 = std::make_shared<TreeCommLevel>(m_comm_1, m_num_up, m_num_down);
}

void TreeCommLevelTest::TearDown()
{
    auto free_func = [] (void *base) { free(base); };
    auto delete_func = [] (size_t ptr) { delete (int*)ptr; };
    EXPECT_CALL(*m_comm_0, barrier());
    EXPECT_CALL(*m_comm_1, barrier());
    EXPECT_CALL(*m_comm_0, window_destroy((size_t)m_sample_window[0]))
        .WillOnce(Invoke(delete_func));
    EXPECT_CALL(*m_comm_1, window_destroy((size_t)m_sample_window[1]))
        .WillOnce(Invoke(delete_func));
    EXPECT_CALL(*m_comm_0, window_destroy((size_t)m_policy_window[0]))
        .WillOnce(Invoke(delete_func));
    EXPECT_CALL(*m_comm_1, window_destroy((size_t)m_policy_window[1]))
        .WillOnce(Invoke(delete_func));

    EXPECT_CALL(*m_comm_0, free_mem(m_sample_mem_0))
        .WillOnce(Invoke(free_func));
    EXPECT_CALL(*m_comm_1, free_mem(m_sample_mem_1))
        .WillOnce(Invoke(free_func));
    EXPECT_CALL(*m_comm_0, free_mem(m_policy_mem_0))
        .WillOnce(Invoke(free_func));
    EXPECT_CALL(*m_comm_1, free_mem(m_policy_mem_1))
        .WillOnce(Invoke(free_func));

    m_level_rank_0.reset();
    m_level_rank_1.reset();
}

TEST_F(TreeCommLevelTest, level_rank)
{
    auto comm = std::make_shared<MockComm>();
    EXPECT_CALL(*comm, num_rank()).WillOnce(Return(m_num_rank));
    EXPECT_CALL(*comm, rank()).WillOnce(Return(42));
    size_t policy_size = sizeof(double) * (m_num_down + 1);
    size_t sample_size = sizeof(double) * m_num_rank * (m_num_up + 1);
    double *policy_mem = (double*)malloc(policy_size);
    double *sample_mem = (double*)malloc(sample_size);
    EXPECT_CALL(*comm, alloc_mem(policy_size, _))
        .WillOnce(SetArgPointee<1>(policy_mem));
    EXPECT_CALL(*comm, alloc_mem(sample_size, _))
        .WillOnce(SetArgPointee<1>(sample_mem));

    EXPECT_CALL(*comm, window_create(policy_size, _));
    EXPECT_CALL(*comm, window_create(0, NULL));
    EXPECT_CALL(*comm, barrier());
    EXPECT_CALL(*comm, window_destroy(_)).Times(2);
    EXPECT_CALL(*comm, free_mem(_)).Times(2)
        .WillRepeatedly(Invoke([] (void *base)
                         { free(base); }));
    // create and destroy level for rank 42
    {
        TreeCommLevel level(comm, m_num_up, m_num_down);
        EXPECT_EQ(42, level.level_rank());
    }
}

TEST_F(TreeCommLevelTest, send_up)
{
    EXPECT_CALL(*m_comm_1, window_lock(_, _, _, _));
    EXPECT_CALL(*m_comm_1, window_unlock(_, _));
    EXPECT_CALL(*m_comm_1, window_put(_, sizeof(double), _, _, _)); // ready flag
    EXPECT_CALL(*m_comm_1, window_put(_, 3 * sizeof(double), _, _, _)); // sample message

    // rank 0 will not send to window
    EXPECT_CALL(*m_comm_0, window_lock(_, _, _, _)).Times(0);
    EXPECT_CALL(*m_comm_0, window_unlock(_, _)).Times(0);
    EXPECT_CALL(*m_comm_0, window_put(_, _, _, _, _)).Times(0);

    std::vector<double> sample {5.5, 6.6, 7.7};
    EXPECT_EQ(0u, m_level_rank_0->overhead_send());
    EXPECT_EQ(0u, m_level_rank_1->overhead_send());
    m_level_rank_0->send_up(sample);
    m_level_rank_1->send_up(sample);
    EXPECT_EQ(0u, m_level_rank_0->overhead_send());
    EXPECT_EQ(4 * sizeof(double), m_level_rank_1->overhead_send());

    // errors
    sample = {8.8, 9.9};
    GEOPM_EXPECT_THROW_MESSAGE(m_level_rank_0->send_up(sample),
                               GEOPM_ERROR_INVALID, "sample vector is not sized correctly");
}

TEST_F(TreeCommLevelTest, send_down)
{
    std::vector<std::vector<double> > policy {{2.2, 3.3}, {2.9, 3.9}, {2.1, 3.1}, {2.0, 3.0}};
    ASSERT_EQ(m_num_rank, (int)policy.size());
    size_t msg_size = sizeof(double) * m_num_down;

    EXPECT_CALL(*m_comm_0, window_lock(_, _, _, _)).Times(m_num_rank - 1);
    EXPECT_CALL(*m_comm_0, window_unlock(_, _)).Times(m_num_rank - 1);
    EXPECT_CALL(*m_comm_0, window_put(_, sizeof(double), _, _, _)).Times(m_num_rank - 1);
    EXPECT_CALL(*m_comm_0, window_put(_, msg_size, _, _, _)).Times(m_num_rank - 1);

    EXPECT_EQ(0u, m_level_rank_0->overhead_send());
    m_level_rank_0->send_down(policy);
    EXPECT_EQ((sizeof(double) + msg_size) * (m_num_rank - 1), m_level_rank_0->overhead_send());

    // errors
#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_level_rank_1->send_down(policy),
                               GEOPM_ERROR_LOGIC, "called from rank not at root of level");
#endif
    policy = {{7.7, 6.6}, {5.5, 4.4}};
    GEOPM_EXPECT_THROW_MESSAGE(m_level_rank_0->send_down(policy),
                               GEOPM_ERROR_INVALID, "policy vector is not sized correctly");
    policy = {{7.7}, {6.6}, {5.5}, {4.4}};
    GEOPM_EXPECT_THROW_MESSAGE(m_level_rank_0->send_down(policy),
                               GEOPM_ERROR_INVALID, "policy vector is not sized correctly");
}

TEST_F(TreeCommLevelTest, receive_up_complete)
{
    std::vector<std::vector<double> > sample {{44.4, 33.3, 22.2},
                                              {41.1, 31.1, 21.1},
                                              {46.6, 36.6, 26.6},
                                              {45.5, 35.5, 25.5}};
    ASSERT_EQ(m_num_rank, (int)sample.size());
    std::vector<std::vector<double> > sample_out(m_num_rank, std::vector<double>(m_num_up, 0.0));

    EXPECT_CALL(*m_comm_0, window_lock(_, false, _, _)); // read
    EXPECT_CALL(*m_comm_0, window_lock(_, true, _, _)); // write
    EXPECT_CALL(*m_comm_0, window_unlock(_, _)).Times(2);
    // mock writing into window
    double complete = 1.0;
    double *curr = m_sample_mem_0;
    auto sample_it = sample.begin();
    for (int rank = 0; rank < m_num_rank; ++rank) {
        memcpy(curr, &complete, sizeof(complete));
        ++curr;
        memcpy(curr, sample_it->data(), m_num_up * sizeof(double));
        curr += m_num_up;
        ++sample_it;
    }

    EXPECT_TRUE(m_level_rank_0->receive_up(sample_out));
    EXPECT_EQ(sample, sample_out);
    // errors
#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_level_rank_1->receive_up(sample_out),
                               GEOPM_ERROR_LOGIC, "called from rank not at root of level");
#endif
}

TEST_F(TreeCommLevelTest, receive_up_incomplete)
{
    std::vector<std::vector<double> > sample {{44.4, 33.3, 22.2},
                                              {41.1, 31.1, 21.1},
                                              {46.6, 36.6, 26.6},
                                              {45.5, 35.5, 25.5}};
    ASSERT_EQ(m_num_rank, (int)sample.size());
    std::vector<std::vector<double> > sample_out(m_num_rank, std::vector<double>(m_num_up, NAN));

    EXPECT_CALL(*m_comm_0, window_lock(_, false, _, _)); // read
    EXPECT_CALL(*m_comm_0, window_unlock(_, _));
    // mock writing into window from sender
    double complete = 0.0;
    double *curr = m_sample_mem_0;
    auto sample_it = sample.begin();
    for (int rank = 0; rank < m_num_rank; ++rank) {
        memcpy(curr, &complete, sizeof(complete));
        ++curr;
        memcpy(curr, sample_it->data(), m_num_up * sizeof(double));
        curr += m_num_up;
        ++sample_it;
    }

    EXPECT_FALSE(m_level_rank_0->receive_up(sample_out));
    for (const auto &ss : sample_out) {
        for (auto tt : ss) {
            EXPECT_TRUE(std::isnan(tt));
        }
    }
}

TEST_F(TreeCommLevelTest, receive_down_complete)
{
    // only rank 1 locks window
    EXPECT_CALL(*m_comm_1, window_lock(_, false, _, _)); // read
    EXPECT_CALL(*m_comm_1, window_unlock(_, _));

    std::vector<double> policy = {77.7, 88.8};

    // mock writing into window from sender
    double complete = 1.0;
    memcpy(m_policy_mem_0, &complete, sizeof(complete));
    memcpy(m_policy_mem_1, &complete, sizeof(complete));
    memcpy(m_policy_mem_0 + 1, policy.data(), sizeof(double) * policy.size());
    memcpy(m_policy_mem_1 + 1, policy.data(), sizeof(double) * policy.size());

    std::vector<double> policy_out;
    EXPECT_TRUE(m_level_rank_0->receive_down(policy_out));
    EXPECT_EQ(policy, policy_out);

    policy_out = {};
    EXPECT_TRUE(m_level_rank_1->receive_down(policy_out));
    EXPECT_EQ(policy, policy_out);
}

TEST_F(TreeCommLevelTest, receive_down_incomplete)
{
    // only rank 1 locks window
    EXPECT_CALL(*m_comm_1, window_lock(_, false, _, _)); // read
    EXPECT_CALL(*m_comm_1, window_unlock(_, _));

    std::vector<double> policy = {77.7, 88.8};

    // mock writing into window from sender
    double complete = 0.0;
    memcpy(m_policy_mem_0, &complete, sizeof(complete));
    memcpy(m_policy_mem_1, &complete, sizeof(complete));
    memcpy(m_policy_mem_0 + 1, policy.data(), sizeof(double) * policy.size());
    memcpy(m_policy_mem_1 + 1, policy.data(), sizeof(double) * policy.size());

    std::vector<double> policy_out;
    EXPECT_FALSE(m_level_rank_0->receive_down(policy_out));
    for (auto pp : policy_out) {
        EXPECT_TRUE(std::isnan(pp));
    }

    policy_out = {};
    EXPECT_FALSE(m_level_rank_1->receive_down(policy_out));
    for (auto pp : policy_out) {
        EXPECT_TRUE(std::isnan(pp));
    }
}
