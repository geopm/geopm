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

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <functional>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "TreeCommunicator.hpp"
#include "MockGlobalPolicy.hpp"
#include "geopm_policy.h"
#include "Exception.hpp"
#include "MockComm.hpp"

#if __INTEL_COMPILER
#pragma warning (disable:1478)
#else   /// GNU build
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

class MyGlobalPolicy : public MockGlobalPolicy
{
    public:
        MyGlobalPolicy (struct geopm_policy_message_s pol_mess)
        {
            EXPECT_CALL(*this, policy_message(testing::_))
                .WillRepeatedly(testing::SetArgReferee<0>(pol_mess));
        }
};

class SingleTreeCommunicatorTest : public :: testing :: Test
{
    public:
        SingleTreeCommunicatorTest();
        ~SingleTreeCommunicatorTest();
    protected:
        const std::string m_ctl_path;
        std::shared_ptr<geopm::SingleTreeCommunicator> m_tcomm;
        std::shared_ptr<MyGlobalPolicy> m_polctl;
};

class TreeCommunicatorTest : public :: testing :: Test
{
    public:
        TreeCommunicatorTest();
        ~TreeCommunicatorTest();
    protected:
        bool is_root_of_level(int ppn1_rank, int level_idx) const;
        const std::vector<std::vector<int>> m_coordinates;
        std::shared_ptr<geopm::TreeCommunicator> m_tcomm;
        std::shared_ptr<MyGlobalPolicy> m_polctl;
        std::vector<int> m_factor;
        int m_ppn1_size;
        std::vector<int> m_ppn1_rank;
        std::vector<int> m_level_size;
};

TreeCommunicatorTest::TreeCommunicatorTest()
    : m_coordinates({{0, 0}, {0, 1}, {1, 0}, {1, 1}})
    , m_tcomm(nullptr)
    , m_polctl(nullptr)
    , m_factor({2, 2})
    , m_ppn1_size(4)
    , m_ppn1_rank({0, 1, 2, 3})
    , m_level_size(m_factor)
{
    struct geopm_policy_message_s start_pol = {GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, 0, 1200, 900};
    m_polctl = std::make_shared<MyGlobalPolicy>(start_pol);
}

TreeCommunicatorTest::~TreeCommunicatorTest()
{
    // TODO there was an unlink call here...
}

bool TreeCommunicatorTest::is_root_of_level(int ppn1_rank, int level_idx) const
{
    // very naive implementation, assumes all levels are equal in size
    return (ppn1_rank % m_factor[level_idx]) == 0;
}

SingleTreeCommunicatorTest::SingleTreeCommunicatorTest()
    : m_tcomm(nullptr)
{
    struct geopm_policy_message_s start_pol = {GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, 0, 1200, 900};
    m_polctl = std::make_shared<MyGlobalPolicy>(start_pol);
}


SingleTreeCommunicatorTest::~SingleTreeCommunicatorTest()
{
}

static void config_ppn1_comm(std::shared_ptr<MockComm> ppn1_comm, int ppn1_size, std::shared_ptr<MockComm> cart_comm)
{
    //num_rank, split, barrier
    EXPECT_CALL(*ppn1_comm, num_rank())
        .WillRepeatedly(testing::Return(ppn1_size));
    EXPECT_CALL(*ppn1_comm, split(testing::_, testing::_, testing::Matcher<bool> (testing::_)))
        .WillOnce(testing::Return(cart_comm));
    EXPECT_CALL(*ppn1_comm, barrier())
        .WillRepeatedly(testing::Return());
}

static void config_cart_comm(std::shared_ptr<MockComm> cart_comm, int ppn1_rank, std::vector<std::vector<int>> coordinates,
                             std::vector<int> level_size,
                             std::function<void (size_t, bool, int, int)> win_lock_lambda,
                             std::function<void (const void *, size_t, int, off_t, size_t)> win_put_lambda, bool config_levels = true)
{
    //rank, coordinate, cart_rank, split
    EXPECT_CALL(*cart_comm, rank())
        .WillOnce(testing::Return(ppn1_rank));
    EXPECT_CALL(*cart_comm, coordinate(testing::_, testing::Matcher<std::vector<int> &>(testing::_)))
        .WillOnce(testing::SetArgReferee<1>(coordinates[ppn1_rank]));

    if (config_levels) {
    EXPECT_CALL(*cart_comm, cart_rank(testing::_))
        .WillRepeatedly(testing::Invoke([coordinates] (const std::vector<int> &coords)
                    {
                        auto it = std::find(coordinates.begin(), coordinates.end(), coords);
                        EXPECT_FALSE(it == coordinates.end());
                        return 0;
                    }));

        testing::Sequence level_seq;
        for (size_t level_idx = 0; level_idx < level_size.size(); level_idx++) {
            //num_rank, rank, split (copy constr), barrier, window_create/destroy,
            //window_lock/unlock/put, alloc/free_mem
            std::shared_ptr<MockComm> level_comm = std::make_shared<MockComm>();
            EXPECT_CALL(*cart_comm, split(testing::Matcher<int> (testing::_), testing::_))
                .InSequence(level_seq)
                .WillOnce(testing::Return(level_comm));
            EXPECT_CALL(*level_comm, barrier())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*level_comm, rank())
                .WillOnce(testing::Return(ppn1_rank % level_size[level_idx]));
            EXPECT_CALL(*level_comm, num_rank())
                .WillRepeatedly(testing::Return(level_size[level_idx]));
            EXPECT_CALL(*level_comm, alloc_mem(testing::_, testing::_))
                .WillRepeatedly(testing::Invoke([] (size_t size, void **base)
                            {
                                *base = malloc(size);
                            }));
            EXPECT_CALL(*level_comm, free_mem(testing::_))
                .WillRepeatedly(testing::Invoke([] (void *base)
                            {
                                free(base);
                            }));
            EXPECT_CALL(*level_comm, window_create(testing::_, testing::_))
                .WillRepeatedly(testing::Invoke([] (size_t size, void *base)
                            {
                                return (size_t) base;
                            }));
            EXPECT_CALL(*level_comm, window_destroy(testing::_))
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*level_comm, window_lock(testing::_, testing::_, testing::_, testing::_))
                .WillRepeatedly(testing::Invoke(win_lock_lambda));
            EXPECT_CALL(*level_comm, window_put(testing::_, testing::_, testing::_, testing::_, testing::_))
                .WillRepeatedly(testing::Invoke(win_put_lambda));
            EXPECT_CALL(*level_comm, window_unlock(testing::_, testing::_))
                .WillRepeatedly(testing::Return());
        }
    }
}

TEST_F(SingleTreeCommunicatorTest, hello)
{
    m_tcomm = std::make_shared<geopm::SingleTreeCommunicator>(m_polctl.get());
    EXPECT_EQ(1, m_tcomm->num_level());
    EXPECT_EQ(0, m_tcomm->root_level());
    EXPECT_EQ(0, m_tcomm->level_rank(0));
    EXPECT_EQ(1, m_tcomm->level_size(0));
    EXPECT_EQ((size_t) 0, m_tcomm->overhead_send());

    struct geopm_policy_message_s exp_pol_mess = GEOPM_POLICY_UNKNOWN, pol_mess = GEOPM_POLICY_UNKNOWN;
    struct geopm_sample_message_s exp_sample_mess = {.region_id = 0xDEADBEEF};
    std::vector<struct geopm_sample_message_s> sample_mess(1, {0});

    m_tcomm->get_sample(0, sample_mess);
    EXPECT_TRUE(geopm_is_sample_equal(&GEOPM_SAMPLE_INVALID, &sample_mess[0]));
    m_tcomm->send_sample(0, exp_sample_mess);
    m_tcomm->get_sample(0, sample_mess);
    EXPECT_TRUE(geopm_is_sample_equal(&exp_sample_mess, &sample_mess[0]));

    m_polctl->policy_message(exp_pol_mess);
    m_tcomm->get_policy(0, pol_mess);
    EXPECT_TRUE(geopm_is_policy_equal(&exp_pol_mess, &pol_mess));
    std::vector<struct geopm_policy_message_s> send_pol(1, GEOPM_POLICY_UNKNOWN);
    m_tcomm->send_policy(0, send_pol);
    m_tcomm->get_policy(0, pol_mess);
    EXPECT_TRUE(geopm_is_policy_equal(&exp_pol_mess, &pol_mess));
    m_tcomm.reset();
}

TEST_F(TreeCommunicatorTest, hello)
{
    auto win_lock_lambda = [] (size_t window_id, bool is_exclusive, int rank, int assert)
    {
        return;
    };
    auto win_put_lambda = [] (const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id)
    {
        return;
    };

    for (auto ppn1_rank : m_ppn1_rank) {
        if (ppn1_rank < 2) {
            std::shared_ptr<MockComm> exp_ppn1_comm = std::make_shared<MockComm>();
            std::shared_ptr<MockComm> exp_cart_comm = std::make_shared<MockComm>();
            config_ppn1_comm(exp_ppn1_comm, m_ppn1_size, exp_cart_comm);
            config_cart_comm(exp_cart_comm, ppn1_rank, m_coordinates, m_level_size, win_lock_lambda, win_put_lambda, false);
            if (!is_root_of_level(ppn1_rank, 0)) {
                EXPECT_THROW(std::make_shared<geopm::TreeCommunicator>(m_factor, m_polctl.get(), exp_ppn1_comm), geopm::Exception);
            }
            else {
                EXPECT_THROW(std::make_shared<geopm::TreeCommunicator>(m_factor, (MyGlobalPolicy *) NULL, exp_ppn1_comm), geopm::Exception);
            }
        }

        std::shared_ptr<MockComm> ppn1_comm = std::make_shared<MockComm>();
        std::shared_ptr<MockComm> cart_comm = std::make_shared<MockComm>();
        config_ppn1_comm(ppn1_comm, m_ppn1_size, cart_comm);
        config_cart_comm(cart_comm, ppn1_rank, m_coordinates, m_level_size, win_lock_lambda, win_put_lambda);
        if (ppn1_rank) {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, (MyGlobalPolicy *)NULL, ppn1_comm);
        }
        else {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, m_polctl.get(), ppn1_comm);
        }

        for (size_t level_idx = 0; level_idx < m_level_size.size() + 1; level_idx++) {
            if (level_idx < m_level_size.size()) {
                EXPECT_EQ((ppn1_rank % m_factor[level_idx]), m_tcomm->level_rank(level_idx));
                EXPECT_EQ(m_level_size[level_idx], m_tcomm->level_size(level_idx));
            }
            else {
                EXPECT_EQ((int) level_idx, m_tcomm->root_level());
                EXPECT_EQ(1, m_tcomm->level_size(level_idx));
            }
        }
        m_tcomm.reset();
    }
}

TEST_F(TreeCommunicatorTest, send_policy_down)
{

    for (auto ppn1_rank : m_ppn1_rank) {
        std::shared_ptr<MockComm> ppn1_comm = std::make_shared<MockComm>();
        std::shared_ptr<MockComm> cart_comm = std::make_shared<MockComm>();
        config_ppn1_comm(ppn1_comm, m_ppn1_size, cart_comm);

        struct geopm_policy_message_s pol_mess;
        auto win_lock_lambda = [&pol_mess] (size_t window_id, bool is_exclusive, int rank, int assert)
        {
            void *base = (void *) window_id;
            if (base) {
                memcpy(base, &pol_mess, sizeof(struct geopm_policy_message_s));
            }
            return;
        };
        auto win_put_lambda = [&pol_mess] (const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id)
        {
            EXPECT_TRUE(geopm_is_policy_equal(&pol_mess, (const struct geopm_policy_message_s *) send_buf));
            return;
        };
        config_cart_comm(cart_comm, ppn1_rank, m_coordinates, m_level_size, win_lock_lambda, win_put_lambda);
        if (ppn1_rank) {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, (MyGlobalPolicy *) NULL, ppn1_comm);
        }
        else {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, m_polctl.get(), ppn1_comm);
        }

        if (ppn1_rank == 0) {
            m_tcomm->get_policy(m_tcomm->root_level(), pol_mess);
        }

        for (size_t level_idx = 0; level_idx < m_factor.size(); level_idx++) {
            m_polctl->policy_message(pol_mess);
            std::vector <struct geopm_policy_message_s> policy(m_level_size[level_idx], pol_mess);
            if (is_root_of_level(ppn1_rank, level_idx)) {
                m_tcomm->send_policy(level_idx, policy);
                size_t overhead = m_tcomm->overhead_send();
                size_t num_mess = overhead / sizeof(struct geopm_policy_message_s);
                if (level_idx) {
                    EXPECT_EQ((size_t) m_factor[level_idx], num_mess);
                }
                else {
                    EXPECT_EQ((size_t) 1, num_mess);
                }
            }
            else {
                EXPECT_THROW(m_tcomm->send_policy(level_idx, policy), geopm::Exception);
            }
            m_tcomm->get_policy(level_idx, pol_mess);
            for (auto pol : policy) {
                EXPECT_TRUE(geopm_is_policy_equal(&pol, &pol_mess));
            }
            pol_mess = GEOPM_POLICY_UNKNOWN;
            if (!is_root_of_level(ppn1_rank, level_idx)) {
                EXPECT_THROW(m_tcomm->get_policy(level_idx, pol_mess), geopm::Exception);
            }
            else {
                m_tcomm->get_policy(level_idx, pol_mess);
                for (auto pol : policy) {
                    EXPECT_TRUE(geopm_is_policy_equal(&pol, &pol_mess));
                }
            }
            EXPECT_THROW(m_tcomm->get_policy(-1, pol_mess), geopm::Exception);
            EXPECT_THROW(m_tcomm->send_policy(-1, {}), geopm::Exception);
        }

        m_tcomm.reset();
    }
}

TEST_F(TreeCommunicatorTest, send_sample_up)
{
    struct geopm_sample_message_s sample_mess;

    for (auto ppn1_rank : m_ppn1_rank) {
        std::shared_ptr<MockComm> ppn1_comm = std::make_shared<MockComm>();
        std::shared_ptr<MockComm> cart_comm = std::make_shared<MockComm>();
        config_ppn1_comm(ppn1_comm, m_ppn1_size, cart_comm);
        auto win_lock_lambda = [&sample_mess] (size_t window_id, bool is_exclusive, int rank, int assert)
        {
            void *base = (void *) window_id;
            if (base) {
                for (int x = 0; x < 2; x++) {
                    memcpy((struct geopm_sample_message_s *) base + x, &sample_mess, sizeof(struct geopm_sample_message_s));
                }
            }
            return;
        };
        auto win_put_lambda = [&sample_mess] (const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id)
        {
            EXPECT_TRUE(geopm_is_sample_equal(&sample_mess, (const geopm_sample_message_s *) send_buf));
            return;
        };
        config_cart_comm(cart_comm, ppn1_rank, m_coordinates, m_level_size, win_lock_lambda, win_put_lambda);
        if (ppn1_rank) {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, (MyGlobalPolicy *) NULL, ppn1_comm);
        }
        else {
            m_tcomm = std::make_shared<geopm::TreeCommunicator>(m_factor, m_polctl.get(), ppn1_comm);
        }

        for (size_t level_idx = 0; level_idx < m_factor.size(); level_idx++) {
            static bool once = true;
            sample_mess.region_id = 0xDEADBEEF;
            std::vector <struct geopm_sample_message_s> sample(m_level_size[level_idx], {0});
            m_tcomm->send_sample(level_idx, sample_mess);
            size_t overhead = m_tcomm->overhead_send();
            size_t num_mess = overhead / sizeof(struct geopm_sample_message_s);
            if (level_idx) {
                if (is_root_of_level(ppn1_rank, level_idx)) {
                    EXPECT_EQ((size_t) 0, num_mess);
                    m_tcomm->get_sample(level_idx, sample);
                    for (int x = 0; x < m_factor[level_idx]; x++) {
                        EXPECT_TRUE(geopm_is_sample_equal(&sample_mess, &sample[x]));
                    }
                    sample_mess.region_id = 0;
                    EXPECT_THROW(m_tcomm->get_sample(level_idx, sample), geopm::Exception);
                    sample.resize(0);
                    EXPECT_THROW(m_tcomm->get_sample(level_idx, sample), geopm::Exception);
                }
                else {
                    EXPECT_EQ((size_t) m_factor[level_idx], num_mess);
                    EXPECT_THROW(m_tcomm->get_sample(level_idx, sample), geopm::Exception);
                }
            }
            else {
                if (is_root_of_level(ppn1_rank, level_idx)) {
                    EXPECT_EQ((size_t) 0, num_mess);
                }
                else {
                    EXPECT_EQ((size_t) 1, num_mess);
                }
                EXPECT_THROW(m_tcomm->get_sample(level_idx, sample), geopm::Exception);
            }
            if (once) {
                EXPECT_THROW(m_tcomm->send_sample(-1, sample_mess), geopm::Exception);
                once = false;
            }
        }

        m_tcomm.reset();
    }
}
