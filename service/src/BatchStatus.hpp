/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BATCHSTATUS_HPP_INCLUDE
#define BATCHSTATUS_HPP_INCLUDE

#include <string>
#include <memory>
#include <unordered_map>

#include "POSIXSignal.hpp"

namespace geopm
{
    class BatchStatus
    {
        public:
            static constexpr char M_MESSAGE_READ = 'r';
            static constexpr char M_MESSAGE_WRITE = 'w';
            static constexpr char M_MESSAGE_CONTINUE = 'c';
            static constexpr char M_MESSAGE_QUIT = 'q';
            static constexpr char M_MESSAGE_TERMINATE = 't';

            BatchStatus() = default;
            virtual ~BatchStatus() = default;
            static std::unique_ptr<BatchStatus> make_unique_server(
                int client_pid,
                const std::string &server_key);
            static std::unique_ptr<BatchStatus> make_unique_client(
                const std::string &server_key);

            /// @brief Send an integer to the other process
            ///
            /// @param msg [in] Message number to send.
            virtual void send_message(char msg) = 0;
            /// @brief Receive any integer from the other process.
            ///
            /// @return Message received
            virtual char receive_message(void) = 0;
            /// @brief Receive specified integer from the other
            ///        process.
            ///
            /// @param expect [in] Value that is expected to be
            ///                    received from other process.
            ///
            /// @throw Exception if a message is received, but the
            ///        message does not match the expected_message.
            virtual void receive_message(char expect) = 0;
    };

    class BatchStatusImp : public BatchStatus
    {
        public:
            BatchStatusImp(int read_fd, int write_fd);
            virtual ~BatchStatusImp() = default;
            void send_message(char msg) override;
            char receive_message(void) override;
            void receive_message(char expect) override;

        protected:
            virtual void open_fifo(void) = 0;
            void check_return(int ret, const std::string &func_name);
            int m_read_fd;
            int m_write_fd;

            // This is the single place where the server prefix is located,
            // which is also accessed by BatchStatusTest.
            static constexpr const char* M_DEFAULT_FIFO_PREFIX =
                "/run/geopm-service/batch-status-";
    };

    class BatchStatusServer : public BatchStatusImp
    {
        public:
            ///
            /// The constructor which is called by the server.
            ///
            BatchStatusServer(int other_pid, const std::string &server_key);
            BatchStatusServer(int other_pid, const std::string &server_key,
                              const std::string &fifo_prefix);
            virtual ~BatchStatusServer();

        private:
            void open_fifo(void) override;
            std::string m_read_fifo_path;
            std::string m_write_fifo_path;
    };

    class BatchStatusClient : public BatchStatusImp
    {
        public:
            ///
            /// The constructor which is called by the client.
            ///
            BatchStatusClient(const std::string &server_key);
            BatchStatusClient(const std::string &server_key,
                              const std::string &fifo_prefix);
            virtual ~BatchStatusClient();

        private:
            void open_fifo(void) override;
            std::string m_read_fifo_path;
            std::string m_write_fifo_path;
    };
}

#endif
