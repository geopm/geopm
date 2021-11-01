/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef BATCHSTATUS_HPP_INCLUDE
#define BATCHSTATUS_HPP_INCLUDE

#include <string>
#include <memory>

namespace geopm
{
    class BatchStatus
    {
        public:
            BatchStatus() = default;
            virtual ~BatchStatus() = default;
            static std::unique_ptr<BatchStatus> make_unique_server(int client_pid,
                                                                   const std::string &server_key);
            static std::unique_ptr<BatchStatus> make_unique_client(const std::string &server_key);
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
            BatchStatusImp(int other_pid, const std::string &server_key,
                           bool is_server);
            BatchStatusImp(int other_pid, const std::string &server_key,
                           bool is_server, const std::string &fifo_prefix);
            virtual ~BatchStatusImp();
            void send_message(char msg) override;
            char receive_message(void) override;
            void receive_message(char expect) override;
        private:
            void check_return(int ret, const std::string &func_name);
            const std::string m_fifo_prefix;
            int m_read_fd;
            int m_write_fd;
    };
}

#endif
