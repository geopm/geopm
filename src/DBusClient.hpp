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

#ifndef DBUSCLIENT_HPP_INCLUDE
#define DBUSCLIENT_HPP_INCLUDE

namespace geopm
{
    class DBusClient
    {
        public:
            DBusClient() = default;
            virtual ~DBusClient();
            virtual int open_session(const std::string &mode);
            virtual void close_session(int session_id);
            /// @brief Calls through the D-Bus interface to create a
            ///        batch server.
            ///
            /// Makes a request to the geopm service to start a batch
            /// session through a binding to the D-Bus interface.
            /// This initiates a call to start_batch_server() by geopmd.
            ///
            /// @param [in] signal_config A vector of requests for
            ///        signals to be sampled.
            /// @param [in] control_config Avector of requests for
            ///        controls to be adjusted.
            virtual void start_batch(const std::vector<geopm_request_s> &signal_config,
                                     const std::vector<geopm_request_s> &control_config) = 0;
            /// @brief Calls through the D-Bus interface to stop a
            ///        batch server.
            ///
            /// Make a request to the geopm service to stop a batch
            /// session through a binding to the D-Bus interface.
            /// This initiates a call to stop_batch_server() by geopmd.
            virtual void stop_batch(void) = 0;
            /// @brief Interface with a running batch server to read
            ///        all of the configured signals.
            ///
            /// Initiates a request with the batch server thread by
            /// sending a SIGCONT realtime signal with the associated
            /// sival_int of 0.  The calling thread then waits for the
            /// server thread to respond with SIGCONT.  It then copies
            /// the data out of the signal shared memory buffer and
            /// returns the result.
            ///
            /// @return A vector with all of the signals that were
            ///         configured when start_batch_client() was
            ///         called.
            virtual std::vector<double> read_batch(void) = 0;
            /// @brief Interface with a running batch server to write
            ///        controls.
            ///
            /// Initiates a request with the batch server thread by
            /// copying the control settings into shared memory and
            /// then sending a SIGCONT realtime signal with the
            /// associated sival_int of 1.
            ///
            /// @param [in] A vector with all of the settings for the
            ///         controls that were configured when
            ///         start_batch_client() was called.
            virtual void write_batch(const std::vector<double> &settings) = 0;
            virtual double read_signal(const std::string &signal_name,
                                       int domain_type,
                                       int domain_idx) = 0;
            virtual void write_control(const std::string &control_name,
                                       int domain_type,
                                       int domain_idx,
                                       double setting) = 0;
    }
}

#endif
