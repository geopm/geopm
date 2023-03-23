/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Comm.hpp"
#include "geopm/Exception.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using geopm::Comm;
using testing::StrEq;
using testing::ElementsAre;
using testing::ElementsAreArray;
using testing::Contains;

class CommNullImpTest : public ::testing::Test
{
    protected:
        std::unique_ptr<Comm> m_comm;
        void SetUp();
        void TearDown();
};

void CommNullImpTest::SetUp()
{
    EXPECT_THAT(Comm::comm_names(), Contains("NullComm"));
    m_comm = Comm::make_unique("NullComm");
}

void CommNullImpTest::TearDown()
{
}

TEST_F(CommNullImpTest, split)
{
    EXPECT_NE(nullptr, m_comm->split());
    EXPECT_NE(nullptr, m_comm->split(1, 1));
    EXPECT_NE(nullptr, m_comm->split("Tag! You're it!", 1));
    EXPECT_NE(nullptr, m_comm->split({1}, {1}, false));
    EXPECT_NE(nullptr, m_comm->split_cart({1}));
}

TEST_F(CommNullImpTest, comm_supported)
{
    EXPECT_TRUE(m_comm->comm_supported("NullComm"));
    EXPECT_FALSE(m_comm->comm_supported("MPIComm"));
    EXPECT_FALSE(m_comm->comm_supported("Anything"));
}

TEST_F(CommNullImpTest, ranks)
{
    // NullComm has a single rank
    EXPECT_EQ(1, m_comm->num_rank());
    EXPECT_EQ(0, m_comm->rank());
    EXPECT_EQ(0, m_comm->cart_rank({0}));
}

TEST_F(CommNullImpTest, dimension_create)
{
    std::vector<int> dimension;
    // Only allow 1 rank
    EXPECT_THROW(m_comm->dimension_create(99, dimension), geopm::Exception);

    // One rank, one dimension
    m_comm->dimension_create(1, dimension);
    EXPECT_THAT(dimension, ElementsAre(1));
}

TEST_F(CommNullImpTest, read_back_memory_writes)
{
    static const unsigned int ALLOC_COUNT = 256;
    static const unsigned int BUFFER_SIZE = 32;
    char expected[BUFFER_SIZE];
    std::vector<char *> allocated_buffers(ALLOC_COUNT);

    // Test that we can fill a bunch of alloc'd buffers then read back the same
    // data without any corruption.
    for (unsigned i = 0; i < ALLOC_COUNT; ++i) {
        m_comm->alloc_mem(BUFFER_SIZE, reinterpret_cast<void **>(&allocated_buffers[i]));
        std::snprintf(allocated_buffers[i], BUFFER_SIZE, "Test %d", i);
    }

    for (unsigned i = 0; i < ALLOC_COUNT; ++i) {
        std::snprintf(expected, BUFFER_SIZE, "Test %d", i);
        EXPECT_THAT(allocated_buffers[i], StrEq(expected))
            << "allocated_buffers[" << i << "]";
    }

    for (unsigned i = 0; i < ALLOC_COUNT; ++i) {
        m_comm->free_mem(allocated_buffers[i]);
        std::snprintf(allocated_buffers[i], BUFFER_SIZE, "Test %d", i);
    }
}

TEST_F(CommNullImpTest, window_creation_and_destruction)
{
    static const unsigned int BUFFER_SIZE = 32;
    void * window = nullptr;

    // Can't create a window on empty memory
    m_comm->alloc_mem(static_cast<size_t>(0), &window);
    EXPECT_EQ(static_cast<size_t>(0), m_comm->window_create(0, window));

    // Can't create a window on memory that wasn't given by m_comm->alloc_mem
    EXPECT_THROW(m_comm->window_create(sizeof window, &window), geopm::Exception);

    // Create and destroy a window
    m_comm->alloc_mem(BUFFER_SIZE, &window);
    auto window_id = m_comm->window_create(BUFFER_SIZE, window);
    m_comm->window_destroy(window_id);

    // Can't destroy the already-destroyed window
    EXPECT_THROW(m_comm->window_destroy(window_id), geopm::Exception);

    // Can't destroy a window that doesn't exist
    EXPECT_THROW(m_comm->window_destroy(1234), geopm::Exception);

    // Can't create a window that was previously created and destroyed
    EXPECT_THROW(m_comm->window_create(BUFFER_SIZE, window), geopm::Exception);

    m_comm->free_mem(window);
}

TEST_F(CommNullImpTest, window_lock)
{
    static const unsigned int BUFFER_SIZE = 32;
    void * window;

    // Can't lock or unlock a window that doesn't exist
    EXPECT_THROW(m_comm->window_lock(1234, false, 0, 0), geopm::Exception);
    EXPECT_THROW(m_comm->window_unlock(1234, 0), geopm::Exception);

    m_comm->alloc_mem(BUFFER_SIZE, &window);
    auto window_id = m_comm->window_create(BUFFER_SIZE, window);

    // NullComm only works with 1 rank
    EXPECT_THROW(m_comm->window_lock(window_id, false, 99 /* rank */, 0), geopm::Exception);
    EXPECT_THROW(m_comm->window_unlock(window_id, 99 /* rank */), geopm::Exception);
    m_comm->window_lock(window_id, false, 0, 0);
    m_comm->window_unlock(window_id, 0);

    m_comm->window_destroy(window_id);
    m_comm->free_mem(window);
}

TEST_F(CommNullImpTest, coordinate)
{
    std::vector<int> coordinate;

    // Only works for 1 rank
    EXPECT_THROW(m_comm->coordinate(10), geopm::Exception);
    EXPECT_THROW(m_comm->coordinate(10, coordinate), geopm::Exception);

    EXPECT_EQ(std::vector<int>{}, m_comm->coordinate(0));
    m_comm->coordinate(0, coordinate);
    EXPECT_EQ(std::vector<int>{}, coordinate);
}

TEST_F(CommNullImpTest, barrier)
{
    // Null barrier for a 1-rank comm is just a no-op. No
    // inputs/outputs/side-effects. Nothing really to test. But I guess we can
    // at least see that this call doesn't hang the test.
    m_comm->barrier();
}

TEST_F(CommNullImpTest, broadcast)
{
    // Only works for 1 rank
    EXPECT_THROW(m_comm->broadcast(nullptr, 0, 1), geopm::Exception);
    m_comm->broadcast(nullptr, 0, 0);
}

TEST_F(CommNullImpTest, test)
{
    EXPECT_TRUE(m_comm->test(true));
    EXPECT_FALSE(m_comm->test(false));
}

TEST_F(CommNullImpTest, reduce_max)
{
    static const unsigned COUNT = 2;
    std::vector<double> senders = {1, 2};
    std::vector<double> receivers(COUNT);
    // Only works for 1 rank
    EXPECT_THROW(m_comm->reduce_max(senders.data(), receivers.data(), COUNT, 99), geopm::Exception);

    receivers = std::vector<double>(COUNT);
    m_comm->reduce_max(senders.data(), receivers.data(), senders.size(), 0);
    EXPECT_THAT(receivers, ElementsAreArray(receivers));
}

TEST_F(CommNullImpTest, gather)
{
    static const unsigned COUNT = 2;
    std::vector<double> senders = {1, 2};
    std::vector<double> receivers(COUNT);
    // Only works for 1 rank
    EXPECT_THROW(m_comm->gather(senders.data(), COUNT, receivers.data(), COUNT, 99), geopm::Exception);
    // Only works if sender size == receiver size
    EXPECT_THROW(m_comm->gather(senders.data(), COUNT, receivers.data(), COUNT-1, 0), geopm::Exception);

    receivers = std::vector<double>(COUNT);
    m_comm->gather(senders.data(), COUNT, receivers.data(), COUNT, 0);
    EXPECT_THAT(receivers, ElementsAreArray(receivers));
}

TEST_F(CommNullImpTest, gatherv)
{
    static const unsigned COUNT = 2;
    std::vector<double> senders = {1, 2};
    std::vector<double> receivers(COUNT);
    // Only works for 1 rank
    EXPECT_THROW(m_comm->gatherv(senders.data(), COUNT, receivers.data(), {COUNT}, {0}, 99), geopm::Exception);
    // Only works if sender size == receiver size
    EXPECT_THROW(m_comm->gatherv(senders.data(), COUNT, receivers.data(), {COUNT-1}, {0}, 0), geopm::Exception);

    receivers = std::vector<double>(COUNT);
    m_comm->gatherv(senders.data(), COUNT, receivers.data(), {COUNT}, {0}, 0);
    EXPECT_THAT(receivers, ElementsAreArray(receivers));
}

TEST_F(CommNullImpTest, window_put)
{
    std::vector<double> senders = {1, 2};
    static const unsigned int BUFFER_SIZE = senders.size() * sizeof senders[0];
    void * window;

    // TODO: All 4 branches of the implementation result in throw. Just change
    //       to an unconditional throw?

    // Can't put on a window that doesn't exist
    EXPECT_THROW(m_comm->window_put(senders.data(), senders.size(), 0, 0, 1234), geopm::Exception);

    m_comm->alloc_mem(BUFFER_SIZE, &window);
    auto window_id = m_comm->window_create(BUFFER_SIZE, window);

    // NullComm only works with 1 rank
    EXPECT_THROW(m_comm->window_put(senders.data(), senders.size(), 123, 0, window_id), geopm::Exception);
    // Can't use an offset that is out of bounds
    EXPECT_THROW(m_comm->window_put(senders.data(), senders.size(), 0, BUFFER_SIZE, window_id), geopm::Exception);

    // All valid inputs
    EXPECT_THROW(m_comm->window_put(senders.data(), senders.size(), 0, 0, window_id), geopm::Exception);

    m_comm->window_destroy(window_id);
    m_comm->free_mem(window);
}

TEST_F(CommNullImpTest, tear_down)
{
    // This is a no-op for NullComm
    m_comm->tear_down();
}
