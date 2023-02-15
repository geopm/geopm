/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_PIO_H_INCLUDE
#define GEOPM_PIO_H_INCLUDE

#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @return the number of signal names that can be indexed with the
///         name_idx parameter to the geopm_pio_signal_name()
///         function.  Any error in loading the platform will result
///         in a negative error code describing the failure.
int geopm_pio_num_signal_name(void);

/// @param [in] name_idx The value of name_idx must be greater than
///        zero and less than the return value from
///        geopm_pio_num_signal_name() or else an error will occur.
///
/// @param [in] result_max At most result_max bytes are written to the
///        result string. If result_max is too small to contain the
///        signal name an error will occur.
///
/// @param [out] result Sets the result string to the name of the
///        signal indexed by name_idx. Providing a string of NAME_MAX
///        length will be sufficient for storing any result.
///
/// @result Zero is returned on success and
///         a negative error code is returned if any error occurs.
int geopm_pio_signal_name(int name_idx,
                          size_t result_max,
                          char *result);

/// @return the number of control names that can be indexed with the
///         name_idx parameter to the geopm_pio_control_name()
///         function.  Any error in loading the platform will result
///         in a negative error code describing the failure.
int geopm_pio_num_control_name(void);

/// @param [in] name_idx The value of name_idx must be greater than
///        zero and less than the return value from
///        geopm_pio_num_control_name() or else an error will occur.
///
/// @param [in] result_max At most result_max bytes are written to the
///        result string. If result_max is too small to contain the
///        control name an error will occur.
///
/// @param [out] result Sets the result string to the name of the
///        control indexed by name_idx. Providing a string of NAME_MAX
///        length will be sufficient for storing any result.
///
/// @result Zero is returned on success and
///         a negative error code is returned if any error occurs.
int geopm_pio_control_name(int name_index,
                           size_t result_max,
                           char *result);

/// @brief Query the domain for the signal with name signal_name.
///
/// @param [in] signal_name A string holding the name of the signal.
///
/// @return one of the geopm_domain_e values signifying the
///         granularity at which the signal is measured.  Will return
///         a negative error code if any error occurs, e.g. a request
///         for a signal_name that is not supported by the platform.
int geopm_pio_signal_domain_type(const char *signal_name);

/// @brief Query the domain for the control with name control_name.
///
/// @param [in] control_name A string holding the name of the control.
///
/// @return one of the geopm_domain_e values signifying the
///         granularity at which the control is measured.  Will return
///         a negative error code if any error occurs, e.g. a request
///         for a control_name that is not supported by the platform.
int geopm_pio_control_domain_type(const char *control_name);

/// @brief Read from the platform and interpret into SI units a signal
///        associated with signal_name and store the value in result.
///        This value is read from the geopm_topo_e domain_type domain
///        indexed by domain_idx.
///
/// @param [in] signal_name A string holding the name of the signal.
///
/// @param [in] doman_type If the signal is native to a domain
///        contained within domain_type, the values from the contained
///        domains are aggregated to form result.
///
/// @param [in] domain_idx Index into the geopm_topo_e domain_type
///        domain.
///
/// @param [out] result The value of the signal.
///
/// @details Calling this function does not modify values stored by
///          calling geopm_pio_read_batch().
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_read_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx,
                          double *result);

/// @brief Interpret the setting in SI units associated with
///        control_name and write it to the platform.  This value is
///        written to the geopm_topo_e domain_type domain indexed by
///        domain_idx.
///
/// @param [in] control_name A string holding the name of the control.
///
/// @param [in] doman_type If the control is native to a domain
///        contained within domain_type, then the setting is written
///        to all contained domains.
///
/// @param [in] domain_idx Index into the geopm_topo_e domain_type
///        domain.
///
/// @param [out] setting The setting in SI units associated with
///        control_name.
///
/// @details Calling this function does not modify values stored by
///          calling geopm_pio_adjust().
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_write_control(const char *control_name,
                            int domain_type,
                            int domain_idx,
                            double setting);

/// @brief Push a signal onto the stack of batch access signals.  The
///        signal is defined by selecting a signal_name from one of
///        the values returned by the geopm_pio_signal_name()
///        function, the domain_type from one of the geopm_domain_e
///        values, and the domain_idx between zero to the value
///        returned by geopm_topo_num_domain(domain_type).
///
/// @details Subsequent calls to the geopm_pio_read_batch() function
///          will read the signal and update the internal state used
///          to store batch signals.  All signals must be pushed onto
///          the stack prior to the first call to
///          geopm_pio_read_batch() or geopm_pio_adjust().  After
///          calls to geopm_pio_read_batch() or geopm_pio_adjust()
///          have been made, signals may be pushed again only after
///          performing a reset by calling geopm_pio_reset() and
///          before calling geopm_pio_read_batch() or
///          geopm_pio_adjust() again.  Attempts to push a signal
///          onto the stack after the first call to
///          geopm_pio_read_batch() or geopm_pio_adjust() (and
///          without performing a reset) or attempts to push a
///          signal_name that is not a value provided by
///          geopm_pio_signal_name() will result in a negative return
///          value.
///
/// @param [in] signal_name The name of the signal, coming from the
///        geopm_pio_signal_name() function.
///
/// @param [in] domain_type The type of the domain from one of the
///        geopm_domain_e values
///
/// @param [in] domain_idx between zero to the value returned by
///        geopm_topo_num_domain(domain_type)
///
/// @return The return value of geopm_pio_push_signal() is an index
///         that can be passed as the sample_idx parameter to
///         geopm_pio_sample() to access the signal value stored in
///         the internal state from the last update.  A distinct
///         signal index will be returned for each unique combination
///         of input parameters.
int geopm_pio_push_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx);

/// @brief Push a control onto the stack of batch access controls.
///        The control is defined by selecting a control_name from one
///        of the values returned by the geopm_pio_control_name()
///        function, the domain_type from one of the geopm_domain_e
///        values, and the domain_idx between zero to the value
///        returned by geopm_topo_num_domain(domain_type).
///
/// @details Subsequent calls to the geopm_pio_write_batch() function
///          access the control values in the internal state and write
///          the values to the hardware.  All controls must be pushed
///          onto the stack prior to the first call to
///          geopm_pio_adjust() or geopm_pio_read_batch().  After
///          calls to geopm_pio_adjust() or geopm_pio_read_batch()
///          have been made, controls may be pushed again only after
///          performing a reset by calling geopm_pio_reset() and
///          before calling geopm_pio_adjust() or
///          geopm_pio_read_batch() again.  Attempts to push a
///          control onto the stack after the first call to
///          geopm_pio_adjust() or geopm_pio_read_batch() (and
///          without performing a reset) or attempts to push a
///          control_name that is not a value provided by
///          geopm_pio_control_name() will result in a negative return
///          value.
///
/// @param [in] control_name The name of the control, coming from the
///        geopm_pio_control_name() function.
///
/// @param [in] domain_type The type of the domain from one of the
///        geopm_domain_e values
///
/// @param [in] domain_idx between zero to the value returned by
///        geopm_topo_num_domain(domain_type)
///
/// @return The return value of geopm_pio_push_control() can be passed
///         to the geopm_pio_adjust() function which will update the
///         internal state used to store batch controls.  A distinct
///         control index will be returned for each unique combination
///         of input parameters.
int geopm_pio_push_control(const char *control_name,
                           int domain_type,
                           int domain_idx);

/// @brief Samples cached value of a single signal that has been
///        pushed via geopm_pio_push_signal() and writes the value
///        into result.
///
/// @details The cached value is updated at the time of call to
///          geopm_pio_read_batch().
///
/// @param [in] signal_idx The signal_idx provided matches the return
///        value from geopm_pio_push_signal() when the signal was
///        pushed.
///
/// @param [out] result The value of the signal.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_sample(int signal_idx,
                     double *result);

/// @brief Updates cached value for single control that has been
///        pushed via geopm_pio_push_control() to the value setting.
///
/// @details The cached value will be written to the platform at time
///          of call to geopm_pio_write_batch().
///
/// @param [in] control_idx The control_idx provided matches the
///        return value from geopm_pio_push_control() when the control
///        was pushed.
///
/// @param [in] setting The value of the control that is being pushed.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_adjust(int control_idx,
                     double setting);

/// @brief Read all push signals from the platform so that the next
///        call to geopm_pio_sample() will reflect the updated data.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_read_batch(void);

/// @brief Write all pushed controls so that values provided to
///        geopm_pio_adjust() are written to the platform.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_write_batch(void);

/// @brief Save the state of all controls so that any subsequent
///        changes made through geopm_pio_write_control() or
///        geopm_pio_write_batch() may be reverted with a call to
///        geopm_pio_restore_control().
///
/// @details The control settings are stored in memory managed by
///          GEOPM.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_save_control(void);

/// @brief Save the state of all controls in the directory so that any
///        subsequent changes made through geopm_pio_write_control()
///        or geopm_pio_write_batch() may be reverted with a call to
///        geopm_pio_restore_control().
///
/// @details The control settings are stored in memory managed by
///          GEOPM.
///
/// @param [in] save_dir The directory where to save the controls.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_save_control_dir(const char *save_dir);

/// @brief Restore the state recorded by the last call to
///        geopm_pio_save_control() so that all subsequent changes
///        made through geopm_pio_write_control or
///        geopm_pio_write_batch() are reverted to their previous
///        settings.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_restore_control(void);

/// @brief Restore the state recorded by the last call to
///        geopm_pio_save_control() in the directory so that all
///        subsequent changes made through geopm_pio_write_control or
///        geopm_pio_write_batch() are reverted to their previous
///        settings.
///
/// @param [in] save_dir The directory where to restore the controls.
///
/// @return If an error occurs then negative error code is returned.
///         Zero is returned upon success.
int geopm_pio_restore_control_dir(const char *save_dir);

/// @param [in] signal_name A string holding the name of the signal.
///
/// @param [in] description_max At most description_max bytes are
///        written to the description string. If description_max is
///        too small to contain the description an error will occur.
///
/// @param [out] description Sets the description string to the signal
///        description associated with signal_name. Providing a string
///        of NAME_MAX length will be sufficient for storing any
///        description.
///
/// @result Zero is returned on success and a negative error code is
///         returned if any error occurs.
int geopm_pio_signal_description(const char *signal_name,
                                 size_t description_max,
                                 char *description);

/// @param [in] control_name A string holding the name of the control.
///
/// @param [in] description_max At most description_max bytes are
///        written to the description string. If description_max is
///        too small to contain the description an error will occur.
///
/// @param [out] description Sets the description string to the control description
///                          associated with control_name. Providing a string of
///                          NAME_MAX length will be sufficient for storing any description.
///
/// @result Zero is returned on success and a negative error code is
///         returned if any error occurs.
int geopm_pio_control_description(const char *control_name,
                                  size_t description_max,
                                  char *description);

/// @brief C interface to get enums associated with a signal name.
///
/// This interface supports DBus PlatformGetSignalInfo method.  This C
/// interface is implemented using several PlatformIO methods unlike
/// the other wrappers in this header.
///
/// @param [in] signal_name Name of signal to query.
///
/// @param [out] aggregation_type One of the Agg::m_type_e enum values
///        describing the way the signal is aggregated.
///
/// @param [out] format_type One of the geopm::string_format_e enums
///        defined in Helper.hpp that defines how to format the signal
///        as a string.
///
/// @param [out] behavior_type One of the IOGroup::m_signal_behavior_e
///        enum values that describes the signals behavior over time.
///
/// @return Zero on success, error value on failure.
int geopm_pio_signal_info(const char *signal_name,
                          int *aggregation_type,
                          int *format_type,
                          int *behavior_type);

struct geopm_request_s;

/// @brief Creates a batch server with the following signals and
///        controls.  It would be an error to create a batch server
///        without any signals or controls.
///
/// @param [in] client_pid The PID of the client process to create the
///        batch server with.
///
/// @param [in] num_signal The number of elements in the array
///        signal_config.
///
/// @param [in] signal_config An array of geopm_request_s elements,
///        each containing the name of the signal, the domain, and the
///        domain index.
///
/// @param [in] num_control The number of elements in the array
///        control_config.
///
/// @param [in] control_config An array of geopm_request_s elements,
///        each containing the name of the control, the domain, and
///        the domain index.
///
/// @param [out] server_pid The PID of the created batch server.
///
/// @param [in] key_size The length of the server_key string.  If
///        key_size is too small to contain the server_key an error
///        will occur.
///
/// @param [out] server_key The key used to identify the server
///        connection: a substring in interprocess shared memory keys
///        used for communication.  Providing a string of NAME_MAX
///        length will be sufficient for storing any server_key.
///
/// @result Zero is returned on success and
///         a negative error code is returned if any error occurs.
int geopm_pio_start_batch_server(int client_pid,
                                 int num_signal,
                                 const struct geopm_request_s *signal_config,
                                 int num_control,
                                 const struct geopm_request_s *control_config,
                                 int *server_pid,
                                 int key_size,
                                 char *server_key);

/// @brief Supports the D-Bus interface for stopping a batch server.
///        Call through to BatchServer::stop_batch()
///
/// @details This function is called directly by geopmd in order to
///          end a batch session and kill the batch server process
///          created by start_batch_server().
///
/// @param server_pid The batch server PID to stop, it tries to find
///        it.  If the PID is not found, it is an error.
///
/// @result Zero is returned on success and a negative error code is
///         returned if any error occurs.
int geopm_pio_stop_batch_server(int server_pid);

/// @brief Format the signal according to the format type specified,
///        and write the output into the result string.
///
/// @param [in] signal The signal to be formatted.
///
/// @param [in] format_type The format type is a string_format_e
///        enumerated value.
///
/// @param [in] result_max At most result_max bytes are written to the
///        result string. If result_max is too small to contain the
///        written output string an error will occur.
///
/// @param [out] result Sets the result string to the value of the
///        signal formatted as specified by the format_type. Providing
///        a string of NAME_MAX length will be sufficient for storing
///        any result.
///
/// @return Zero on success, error value on failure.
int geopm_pio_format_signal(double signal,
                            int format_type,
                            size_t result_max,
                            char *result);

/// @brief Reset the GEOPM platform interface causing resources to be
///        freed.  This will cause the internal PlatormIO instance to
///        be released/deleted and reconstructed.  As a result, any
///        signals and controls that had been pushed will be cleared,
///        any batch servers that had been started will be stopped,
///        and all registered IOGroups will be reset.
///
///        NOTE: the reset only applies to the Service PlatformIO
///        instance and does not affect the PlatformIO instance
///        managed by the GEOPM HPC runtime.
void geopm_pio_reset(void);

/// @param [in] value Check if the given parameter is a valid value.
///
/// @return 0 if the value is valid, GEOPM_ERROR_INVALID if the value is invalid.
int geopm_pio_check_valid_value(double value);

/// @brief Discover the thread PIDS associated with an application
///
/// Called by a profiling application (like geopmctl) to determine
/// which Linux PIDs should be tracked as part of an application.
///
/// @param [in] profile_name String that identifies the application
///        being profiled.
///
/// @param [in] max_num_pid Number of integers allocated for the pid
///        array.
///
/// @param [out] num_pid Actual number of elements written to the pid
///        array.
///
/// @param [out] pid An array of Linux PIDs that are associated with
///        the application.
int geopm_pio_profile_pids(const char *profile_name, int max_num_pid, int *num_pid, int *pid);


#ifdef __cplusplus
}
#endif
#endif
