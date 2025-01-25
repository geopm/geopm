// FILE: TestBatchServer.cpp

#include <unistd.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "BatchServer.hpp"
#include "BatchStatus.hpp"
#include "POSIXSignal.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/PlatformIO.hpp"
#include "MockPlatformIO.hpp"
#include "MockBatchStatus.hpp"
#include "MockPOSIXSignal.hpp"

using namespace geopm;
using namespace testing;

class MockSharedMemoryPure : public SharedMemory
{
    public:
        MOCK_METHOD(void *, pointer, (), (const, override));
        MOCK_METHOD(std::string, key, (), (const, override));
        MOCK_METHOD(size_t, size, (), (const, override));
        MOCK_METHOD(std::unique_ptr<geopm::SharedMemoryScopedLock>,
                    get_scoped_lock, (), (override));
        MOCK_METHOD(void, unlink, (), (override));
        MOCK_METHOD(void, chown, (const unsigned int gid, const unsigned int uid),
                    (const, override));

};

class BatchServerTest : public ::testing::Test
{
    protected:
        void SetUp(void) override;
        void check_push_requests(void);

        int m_client_pid;
        std::vector<geopm_request_s> m_signal_config;
        std::vector<geopm_request_s> m_control_config;
        std::string m_signal_shmem_key;
        std::string m_control_shmem_key;
        int m_server_pid;

        std::shared_ptr<MockPlatformIO> m_pio;
        std::shared_ptr<MockBatchStatus> m_batch_status;
        std::shared_ptr<MockPOSIXSignal> m_posix_signal;
        std::shared_ptr<MockSharedMemoryPure> m_signal_shmem;
        std::shared_ptr<MockSharedMemoryPure> m_control_shmem;

        std::shared_ptr<BatchServerImp> m_batch_server;
};

void BatchServerTest::SetUp(void)
{
    m_client_pid = 1234;
    m_signal_config = {geopm_request_s {0, 0, "signal_name"}};
    m_control_config = {geopm_request_s {0, 0, "control_name"}};
    m_signal_shmem_key = "signal_shmem_key";
    m_control_shmem_key = "control_shmem_key";
    m_server_pid = 5678;

    m_pio = std::make_shared<MockPlatformIO>();
    m_batch_status = std::make_shared<MockBatchStatus>();
    m_posix_signal = std::make_shared<MockPOSIXSignal>();
    m_signal_shmem = std::make_shared<MockSharedMemoryPure>();
    m_control_shmem = std::make_shared<MockSharedMemoryPure>();

    EXPECT_CALL(*m_signal_shmem, unlink()).
        WillRepeatedly(Return());
    EXPECT_CALL(*m_control_shmem, unlink()).
        WillRepeatedly(Return());
    m_batch_server = std::make_shared<BatchServerImp>(
        m_client_pid, m_signal_config, m_control_config,
        m_signal_shmem_key, m_control_shmem_key,
        *m_pio, m_batch_status, m_posix_signal,
        m_signal_shmem, m_control_shmem, m_server_pid);
}

TEST_F(BatchServerTest, constructor)
{
    EXPECT_EQ(m_server_pid, m_batch_server->server_pid());
    EXPECT_EQ(std::to_string(m_client_pid), m_batch_server->server_key());
}

TEST_F(BatchServerTest, read_message)
{
    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_READ));

    char message = m_batch_server->read_message();
    EXPECT_EQ(BatchStatus::M_MESSAGE_READ, message);
}

TEST_F(BatchServerTest, write_message)
{
    EXPECT_CALL(*m_batch_status, send_message(BatchStatus::M_MESSAGE_CONTINUE));

    m_batch_server->write_message(BatchStatus::M_MESSAGE_CONTINUE);
}

TEST_F(BatchServerTest, run_batch)
{
    EXPECT_CALL(*m_pio, push_signal(_, _, _)).Times(1);
    EXPECT_CALL(*m_pio, push_control(_, _, _)).Times(1);
    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_TERMINATE));

    m_batch_server->run_batch();
}

TEST_F(BatchServerTest, event_loop)
{
    EXPECT_CALL(*m_batch_status, receive_message())
        .WillOnce(Return(BatchStatus::M_MESSAGE_TERMINATE));

    m_batch_server->event_loop();
}

void BatchServerTest::check_push_requests(void)
{

    EXPECT_CALL(*m_pio, push_signal(_, _, _)).Times(1);
    EXPECT_CALL(*m_pio, push_control(_, _, _)).Times(1);

    m_batch_server->push_requests();
}

TEST_F(BatchServerTest, push_requests)
{
    check_push_requests();
}

TEST_F(BatchServerTest, read_and_update)
{
    check_push_requests();
    double buffer[1] = {0.0};
    EXPECT_CALL(*m_signal_shmem, pointer())
        .WillOnce(Return(buffer));
    EXPECT_CALL(*m_pio, read_batch());
    EXPECT_CALL(*m_pio, sample(_))
        .WillOnce(Return(1.0));

    m_batch_server->read_and_update();
    EXPECT_EQ(1.0, buffer[0]);
}

TEST_F(BatchServerTest, update_and_write)
{
    check_push_requests();
    double buffer[1] = {1.0};
    EXPECT_CALL(*m_control_shmem, pointer())
        .WillOnce(Return(buffer));
    EXPECT_CALL(*m_pio, adjust(_, 1.0));
    EXPECT_CALL(*m_pio, write_batch());

    m_batch_server->update_and_write();
}

TEST_F(BatchServerTest, register_handler)
{
    EXPECT_CALL(*m_posix_signal, make_sigset(_));
    EXPECT_CALL(*m_posix_signal, sig_action(_, _, _));

    m_batch_server->register_handler();
}
