/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            /// The BatchClient interface is used to communicate with an
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
