/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BATCHSERVER_HPP_INCLUDE
#define BATCHSERVER_HPP_INCLUDE

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <signal.h>


struct geopm_request_s;

namespace geopm
{
    class PlatformIO;
    class SharedMemory;
    class BatchStatus;
    class POSIXSignal;

    class BatchServer
    {
        public:
            /// @brief Interface called by geopmd to create the server
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
            ///     "<prefix>/geopm-service-batch-buffer-<KEY>-signals"
            ///     "<prefix>/geopm-service-batch-buffer-<KEY>-controls"
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
            /// @return The shm key to use for the signal shared memory
            ///         region.
            static std::string get_signal_shmem_key(
                const std::string &server_key);
            /// @return The shm key to use for the control shared memory
            ///         region.
            static std::string get_control_shmem_key(
                const std::string &server_key);
            /// @return The Unix process ID of the server process
            ///        created.
            static int main(int argc, char **argv);
            static void main(int client_pid, std::istream &input_stream);
            virtual int server_pid(void) const = 0;
            /// @return The key used to identify the server
            ///        connection: a substring in interprocess shared
            ///        memory keys used for communication.
            virtual std::string server_key(void) const = 0;
            /// @brief Returns true if the batch server is running
            virtual bool is_active(void) = 0;
            virtual void run_batch(void) = 0;
            virtual void create_shmem(void) = 0;
            virtual void register_handler(void) = 0;
        protected:
            static constexpr const char* M_SHMEM_PREFIX =
                "/run/geopm/batch-buffer-";
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
                           const std::string &signal_shmem_key,
                           const std::string &control_shmem_key,
                           PlatformIO &pio,
                           std::shared_ptr<BatchStatus> batch_status,
                           std::shared_ptr<POSIXSignal> posix_signal,
                           std::shared_ptr<SharedMemory> signal_shmem,
                           std::shared_ptr<SharedMemory> control_shmem,
                           int server_pid);
            BatchServerImp(const BatchServerImp &other) = delete;
            BatchServerImp &operator=(const BatchServerImp &other) = delete;
            virtual ~BatchServerImp();
            int server_pid(void) const override;
            std::string server_key(void) const override;
            bool is_active(void) override;
            void run_batch(void) override;
            void create_shmem(void) override;
            void register_handler(void) override;
        private:
            void push_requests(void);
            void read_and_update(void);
            void update_and_write(void);
            void check_invalid_signal(void);
            void check_return(int ret, const std::string &func_name) const;
            char read_message(void);
            void write_message(char message);
            void event_loop(void);

            const int m_client_pid;
            const std::string m_server_key;
            const std::vector<geopm_request_s> m_signal_config;
            const std::vector<geopm_request_s> m_control_config;
            const std::string m_signal_shmem_key;
            const std::string m_control_shmem_key;
            PlatformIO &m_pio;
            std::shared_ptr<SharedMemory> m_signal_shmem;
            std::shared_ptr<SharedMemory> m_control_shmem;
            std::shared_ptr<BatchStatus> m_batch_status;
            std::shared_ptr<POSIXSignal> m_posix_signal;
            int m_server_pid;
            bool m_is_active;
            bool m_is_client_attached;
            bool m_is_client_waiting;
            /// @brief Stores the PlatformIO batch handles for all pushed
            ///        signals
            std::vector<int> m_signal_handle;
            /// @brief Stores the PlatformIO batch handles for all pushed
            ///        controls
            std::vector<int> m_control_handle;
    };
}

#endif
