/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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

#ifndef BATCHCLIENT_HPP_INCLUDE
#define BATCHCLIENT_HPP_INCLUDE

#include <memory>
#include <vector>
#include <signal.h>

struct geopm_request_s;

namespace geopm
{
    class SharedMemory;
    class BatchStatus;

    /// @brief Interface that will attach to a batch server.  The batch server
    ///        that it connects to is typically created through a call to the
    ///        GEOPM DBus interface io.github.geopm.PlatformStartBatch.
    class BatchClient
    {
        public:
            BatchClient() = default;
            virtual ~BatchClient() = default;

            /// @brief Factory method to create a pointer to a BatchClient object
            ///
            /// The BatchClient interface is used to communcate with an
            /// existing GEOPM batch server. The batch server is typically
            /// created with the PlatformStartBatch GEOPM DBus interface.
            ///
            /// @param server_key [in] The server key that was returned when
            ///                   the batch server was created.
            ///
            /// @param timeout [in] Maximum wait time to attach to batch
            ///                server in units of seconds.
            ///
            /// @param num_signal [in] Number of signal requests made when
            ///                   starting the batch server.
            ///
            /// @param num_signal [in] Number of control requests made when
            ///                   starting the batch server.
            ///
            /// @return New unique pointer to an object that supports the
            ///         BatchClient interface.
            static std::unique_ptr<BatchClient> make_unique(const std::string &server_key,
                                                            double timeout,
                                                            int num_signal,
                                                            int num_control);

            /// @brief Ask batch server to read all signal values and return
            ///        result.
            ///
            /// Command is issued to batch server to read all pushed signal
            /// values.  All of the values read by the batch server are
            /// returned.
            ///
            /// @return A vector containing all values read by batch server
            ///
            virtual std::vector<double> read_batch(void) = 0;

            /// @brief Ask batch server to write all of the control values.
            ///
            /// Sends the vector of settings to the batch server.  This
            /// function blocks until the batch server has written all values.
            ///
            /// @param settings [in] Control settings to be written: one for
            ///                 each control requests made when batch server
            ///                 was created.
            virtual void write_batch(std::vector<double> settings) = 0;

            /// @brief Send message to batch server asking it to quit.
            virtual void stop_batch(void) = 0;
    };

    class BatchClientImp : public BatchClient
    {
        public:
            BatchClientImp(const std::string &server_key, double timeout,
                           int num_signal, int num_control);
            BatchClientImp(int num_signal, int num_control,
                           std::shared_ptr<BatchStatus> batch_status,
                           std::shared_ptr<SharedMemory> signal_shmem,
                           std::shared_ptr<SharedMemory> control_shmem);
            virtual ~BatchClientImp() = default;
            std::vector<double> read_batch(void) override;
            void write_batch(std::vector<double> settings) override;
            void stop_batch(void) override;
        private:
            int m_num_signal;
            int m_num_control;
            std::shared_ptr<BatchStatus> m_batch_status;
            std::shared_ptr<SharedMemory> m_signal_shmem;
            std::shared_ptr<SharedMemory> m_control_shmem;
    };
}

#endif
