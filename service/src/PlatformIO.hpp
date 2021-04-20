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

#ifndef PLATFORMIO_HPP_INCLUDE
#define PLATFORMIO_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <set>

#include "geopm_pio.h"

namespace geopm
{
    class IOGroup;
    class ProfileIOGroup;

    /// @brief Class which is a collection of all valid control and
    /// signal objects for a platform
    class PlatformIO
    {
        public:
            PlatformIO() = default;
            virtual ~PlatformIO() = default;
            /// @brief Registers an IOGroup with the PlatformIO so
            ///        that its signals and controls are available
            ///        through the PlatformIO interface.
            /// @param [in] iogroup Shared pointer to the IOGroup
            ///        object.
            virtual void register_iogroup(std::shared_ptr<IOGroup> iogroup) = 0;
            /// @brief Returns the names of all available signals.
            ///        This includes all signals and aliases provided
            ///        by IOGroups as well as signals provided by
            ///        PlatformIO itself.
            virtual std::set<std::string> signal_names(void) const = 0;
            /// @brief Returns the names of all available controls.
            ///        This includes all controls and aliases provided
            ///        by IOGroups as well as controls provided by
            ///        PlatformIO itself.
            virtual std::set<std::string> control_names(void) const = 0;
            /// @brief Query the domain for a named signal.
            /// @param [in] signal_name The name of the signal.
            /// @return One of the PlatformTopo::m_domain_e values
            ///         signifying the granularity at which the signal
            ///         is measured.  Will return M_DOMAIN_INVALID if
            ///         the signal name is not supported.
            virtual int signal_domain_type(const std::string &signal_name) const = 0;
            /// @brief Query the domain for a named control.
            /// @param [in] control_name The name of the control.
            /// @return One of the PlatformTopo::m_domain_e values
            ///         signifying the granularity at which the
            ///         control can be adjusted.  Will return
            ///         M_DOMAIN_INVALID if the signal name is not
            ///         supported.
            virtual int control_domain_type(const std::string &control_name) const = 0;
            /// @brief Push a signal onto the end of the vector that
            ///        can be sampled.
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of signal when sample() method is called
            ///         or throws if the signal is not valid
            ///         on the platform.  Returned signal index will be
            ///         repeated for each unique tuple of push_signal
            ///         input parameters.
            virtual int push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Push a control onto the end of the vector that
            ///        can be adjusted.
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of the control if the requested control
            ///         is valid, otherwise throws.  Returned control index
            ///         will be repeated for each unique tuple of the push_control
            ///         input parameters.
            virtual int push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx) = 0;
            /// @brief Sample a single signal that has been pushed on
            ///        to the signal stack.  Must be called after a call
            ///        to read_batch(void) method which updates the state
            ///        of all signals.
            /// @param [in] signal_idx index returned by a previous call
            ///        to the push_signal() method.
            /// @return Signal value measured from the platform in SI units.
            virtual double sample(int signal_idx) = 0;
            /// @brief Adjust a single control that has been pushed on
            ///        to the control stack.  This control will not
            ///        take effect until the next call to
            ///        write_batch(void).
            /// @param [in] control_idx Index of control to be adjusted
            ///        returned by a previous call to the push_control() method.
            /// @param [in] setting Value of control parameter in SI units.
            virtual void adjust(int control_idx,
                                double setting) = 0;
            /// @brief Read all pushed signals so that the next call
            ///        to sample() will reflect the updated data.
            virtual void read_batch(void) = 0;
            /// @brief Write all of the pushed controls so that values
            ///        previously given to adjust() are written to the
            ///        platform.
            virtual void write_batch(void) = 0;
            /// @brief Read from platform and interpret into SI units
            ///        a signal given its name and domain.  Does not
            ///        modify the values stored by calling
            ///        read_batch().
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        PlatformTopo::m_domain_e enum described in
            ///        PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return The value in SI units of the signal.
            virtual double read_signal(const std::string &signal_name,
                                       int domain_type,
                                       int domain_idx) = 0;
            /// @brief Interpret the setting and write setting to the
            ///        platform.  Does not modify the values stored by
            ///        calling adjust().
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        PlatformTopo::m_domain_e enum described in
            ///        PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @param [in] setting Value in SI units of the setting
            ///        for the control.
            virtual void write_control(const std::string &control_name,
                                       int domain_type,
                                       int domain_idx,
                                       double setting) = 0;
            /// @brief Save the state of all controls so that any
            ///        subsequent changes made through PlatformIO
            ///        can be undone with a call to the restore_control()
            ///        method.
            virtual void save_control(void) = 0;
            /// @brief Restore all controls to values recorded in
            ///        previous call to the save_control() method.
            virtual void restore_control(void) = 0;
            /// @brief Save the state of all controls so that any
            ///        subsequent changes made through PlatformIO can
            ///        be undone with a call to the restore_control()
            ///        method.  Each IOGroup that supports controls
            ///        will populate one file in the save directory
            ///        that contains the saved state and name the file
            ///        after the IOGroup name.
            /// @param [in] save_dir Directory to be populated with
            ///        save files.
            virtual void save_control(const std::string &save_dir) = 0;
            /// @brief Restore all controls to values recorded in
            ///        previous call to the save_control(save_dir)
            ///        method.  The directory provided contains the
            ///        result of the previous saved state.
            /// @param [in] save_dir Directory populated with save
            ///        files.
            virtual void restore_control(const std::string &save_dir) = 0;
            /// @brief Supports the D-Bus interface for starting a
            ///        batch server.
            ///
            /// This function is called directly by geopmd in order to
            /// fork a new process that will support calls within the
            /// client_pid to read_batch_client() and
            /// write_batch_client().  The client initiates the server
            /// by calling start_batch_client() within the client_pid
            /// which make the request through D-Bus to start the
            /// server.  The server_pid and server_key are stored in
            /// the client's PlatformIO object to enable interactions
            /// with the server while the batch session is open.
            ///
            /// @param [in] client_pid The Unix process ID of the
            ///        client process that is initiating the batch
            ///        server.
            /// @param [in] signal_config A vector of requests for
            ///        signals to be sampled.
            /// @param [in] control_config Avector of requests for
            ///        controls to be adjusted.
            /// @param [out] server_pid The Unix process ID of the
            ///        server process created.
            /// @param [out] server_key The key used to identify the
            ///        server connection: a substring in interprocess
            ///        shared memory keys used for communication.
            virtual void start_batch_server(int client_pid,
                                            std::vector<geopm_request_s> signal_config,
                                            std::vector<geopm_request_s> control_config,
                                            int &server_pid,
                                            std::string &server_key) = 0;
            /// @brief Supports the D-Bus interface for stopping a
            ///        batch server.
            ///
            /// This function is called directly by geopmd in order to
            /// end a batch session and kill the batch server process
            /// created by start_batch_server().
            ///
            /// @param [in] server_pid The Unix process ID of the
            ///        server process returned by a previous call to
            ///        start_batch_server().
            virtual void stop_batch_server(int server_pid) = 0;
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
            virtual void start_batch_client(std::vector<geopm_request_s> signal_config,
                                            std::vector<geopm_request_s> control_config) = 0;
            /// @brief Calls through the D-Bus interface to stop a
            ///        batch server.
            ///
            /// Make a request to the geopm service to stop a batch
            /// session through a binding to the D-Bus interface.
            /// This initiates a call to stop_batch_server() by geopmd.
            virtual void stop_batch_client(void) = 0;
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
            virtual std::vector<double> read_batch_client(void) = 0;
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
            virtual void write_batch_client(std::vector<double> settings) = 0;
            /// @brief Returns a function appropriate for aggregating
            ///        multiple values of the given signal into a
            ///        single value.
            /// @param [in] signal_name Name of the signal.
            /// @return A function from vector<double> to double.
            virtual std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const = 0;
            /// @brief Returns a function that can be used to convert
            ///        a signal of the given name into a printable
            ///        string.
            /// @param [in] signal_name Name of the signal.
            /// @return A function from double to formatted std::string.
            virtual std::function<std::string(double)> format_function(const std::string &signal_name) const = 0;
            /// @brief Returns a description of the signal.  This
            ///        string can be used by tools to generate help
            ///        text for users of PlatformIO.
            virtual std::string signal_description(const std::string &signal_name) const = 0;
            /// @brief Returns a description of the control.  This
            ///        string can be used by tools to generate help
            ///        text for users of PlatformIO.
            virtual std::string control_description(const std::string &control_name) const = 0;
            /// @brief Returns a hint about how a signal will change
            ///        as a function of time.
            ///
            /// This can be used when generating reports to decide how
            /// to summarize a signal's value for the entire
            /// application run.
            ///
            /// @param [in] signal_name Name of the signal.
            virtual int signal_behavior(const std::string &signal_name) const = 0;
            /// @brief Interace called by geopmd to create the server
            ///        side of a session interface.
            ///
            /// This is the underlying implemetation for the
            /// io.github.geopm.PlatformOpenSession API.  This method
            /// is called by geopmd to create a pthread to support a
            /// new session.  Calling this method will create a new
            /// child thread of the calling process (the geopmd daemon
            /// process) that will interace with the client thread
            /// (client_pid) to provide access to the requested
            /// signals (signal_config) and controls (control_config).
            /// The method will return after the shared memory regions
            /// supporting the service have been created and the child
            /// thread that updates those regions has begun updating
            /// the signal buffer.  Access is provided through the
            /// SharedMemory interface with two shm file descriptors
            /// created, one for signals and one for controls.  The
            /// shm keys created will be of the form:
            ///
            ///     "/geopm-service-<KEY>-signals"
            ///     "/geopm-service-<KEY>-controls"
            ///
            /// where <KEY> is the "key" field in the returned
            /// geopm_session_s structure.  This key is used by the
            /// be used on the client side with the
            /// SharedMemory::make_unique_user() as the shm_key
            /// parameter.

            /// update the client process with all of the signals in
            /// the configuration every
            ///
            ///
            virtual struct geopm_session_s open_session_server(int client_pid,
                                                               std::vector<struct geopm_request_s> signal_config,
                                                               std::vector<struct geopm_request_s> control_config,
                                                               double interval,
                                                               int protocol) = 0;

            /// @brief Calls the D-Bus interface to create a client session
            virtual struct geopm_session_s open_session_client(std::vector<struct geopm_request_s> signal_config,
                                                               std::vector<struct geopm_request_s> control_config,
                                                               double interval,
                                                               int protocol) = 0;


            virtual void close_session(const std::string &key) = 0;
    };

    PlatformIO &platform_io(void);
}

#endif
