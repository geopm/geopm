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

#ifndef BATCHSERVER_HPP_INCLUDE
#define BATCHSERVER_HPP_INCLUDE

#include <memory>
#include <vector>
#include <signal.h>


struct geopm_request_s;

namespace geopm
{
    class PlatformIO;
    class SharedMemory;
    class POSIXSignal;

    class BatchServer
    {
        public:
            enum m_message_e {
                M_MESSAGE_READ = 0x10FF00,
                M_MESSAGE_WRITE,
                M_MESSAGE_READY,
                M_MESSAGE_QUIT,
            };

            /// @brief Interace called by geopmd to create the server
            ///        for batch commands.
            BatchServer() = default;
            virtual ~BatchServer() = default;
            /// @brief Supports the D-Bus interface for starting a
            ///        batch server.
            ///
            /// This function is called directly by geopmd in order to
            /// fork a new process that will support calls within the
            /// client_pid to read_batch_client() and
            /// write_batch_client().  The client initiates the server
            /// by calling start_batch_client() within the client_pid
            /// which make the request through D-Bus to start the
            /// server.  The server_pid and server_key are stored by
            /// the client to enable interactions with the server
            /// while the batch session is open.
            ///
            /// The method will return after the shared memory regions
            /// supporting the service have been created and the child
            /// thread that updates those regions is waiting for a
            /// signal.  Access is provided through the SharedMemory
            /// interface with two shm file descriptors created, one
            /// for signals and one for controls.  The shm keys
            /// created will be of the form:
            ///
            ///     "/geopm-service-<KEY>-signals"
            ///     "/geopm-service-<KEY>-controls"
            ///
            /// where <KEY> is the "server_key".  This key is used by
            /// the client side with the
            /// SharedMemory::make_unique_user() as the shm_key
            /// parameter.
            ///
            /// @param [in] client_pid The Unix process ID of the
            ///        client process that is initiating the batch
            ///        server.
            /// @param [in] signal_config A vector of requests for
            ///        signals to be sampled.
            /// @param [in] control_config Avector of requests for
            ///        controls to be adjusted.
            static std::unique_ptr<BatchServer> make_unique(int client_pid,
                                                           const std::vector<geopm_request_s> &signal_config,
                                                           const std::vector<geopm_request_s> &control_config);
            /// @return The Unix process ID of the server process
            ///        created.
            virtual int server_pid(void) const = 0;
            /// @return The key used to identify the server
            ///        connection: a substring in interprocess shared
            ///        memory keys used for communication.
            virtual std::string server_key(void) const = 0;
            /// @brief Supports the D-Bus interface for stopping a
            ///        batch server.
            ///
            /// This function is called directly by geopmd in order to
            /// end a batch session and kill the batch server process
            /// created by start_batch_server().
            virtual void stop_batch(void) = 0;
            virtual bool is_active(void) const = 0;
    };

    class BatchServerImp : public BatchServer
    {
        public:
            BatchServerImp(int client_pid,
                          const std::vector<geopm_request_s> &signal_config,
                          const std::vector<geopm_request_s> &control_config);
            BatchServerImp(int client_pid,
                          const std::vector<geopm_request_s> &signal_config,
                          const std::vector<geopm_request_s> &control_config,
                          PlatformIO &pio,
                          std::shared_ptr<POSIXSignal> posix_signal,
                          std::shared_ptr<SharedMemory> signal_shmem,
                          std::shared_ptr<SharedMemory> control_shmem);
            virtual ~BatchServerImp();
            int server_pid(void) const override;
            std::string server_key(void) const override;
            void stop_batch(void) override;
            void run_batch(int parent_pid);
            bool is_active(void) const override;
        private:
            void push_requests(void);
            void read_and_update(void);
            void update_and_write(void);
            void critical_region_enter(void);
            void critical_region_exit(void);
            void create_shmem(void);
            void check_invalid_signal(void);

            int m_client_pid;
            std::vector<geopm_request_s> m_signal_config;
            std::vector<geopm_request_s> m_control_config;
            PlatformIO &m_pio;
            std::shared_ptr<SharedMemory> m_signal_shmem;
            std::shared_ptr<SharedMemory> m_control_shmem;
            std::shared_ptr<POSIXSignal> m_posix_signal;
            std::string m_server_key;
            int m_server_pid;
            bool m_is_active;
            std::vector<int> m_signal_idx;
            std::vector<int> m_control_idx;
            bool m_is_blocked;
            sigset_t m_orig_mask;
            sigset_t m_block_mask;
            struct sigaction m_orig_action_sigio;
            struct sigaction m_orig_action_sigcont;
            struct sigaction m_orig_action_sigquit;
            struct sigaction m_orig_action_sigchld;
    };
}

#endif
