/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORMIO_HPP_INCLUDE
#define PLATFORMIO_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <set>
#include <climits>

#include "geopm_pio.h"

struct geopm_request_s {
    int domain_type;
    int domain_idx;
    char name[NAME_MAX];
};

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
            ///
            /// @details This method provides the mechanism for extending
            ///          the PlatformIO interface at runtime.
            ///
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
            ///
            /// @param [in] signal_name The name of the signal.
            ///
            /// @return One of the geopm_domain_e values
            ///         signifying the granularity at which the signal
            ///         is measured.  Will return GEOPM_DOMAIN_INVALID if
            ///         the signal name is not supported.
            virtual int signal_domain_type(const std::string &signal_name) const = 0;
            /// @brief Query the domain for a named control.
            ///
            /// @param [in] control_name The name of the control.
            ///
            /// @return One of the geopm_domain_e values
            ///         signifying the granularity at which the
            ///         control can be adjusted.  Will return
            ///         GEOPM_DOMAIN_INVALID if the signal name is not
            ///         supported.
            virtual int control_domain_type(const std::string &control_name) const = 0;
            /// @brief Push a signal onto the end of the vector that
            ///        can be sampled.
            ///
            /// @param [in] signal_name Name of the signal requested.
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
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
            ///
            /// @param [in] control_name Name of the control requested.
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
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
            ///
            /// @param [in] signal_idx index returned by a previous call
            ///        to the push_signal() method.
            ///
            /// @return Signal value measured from the platform in SI units.
            virtual double sample(int signal_idx) = 0;
            /// @brief Adjust a single control that has been pushed on
            ///        to the control stack.  This control will not
            ///        take effect until the next call to
            ///        write_batch(void).
            ///
            /// @param [in] control_idx Index of control to be adjusted
            ///        returned by a previous call to the push_control() method.
            ///
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
            ///
            /// @param [in] signal_name Name of the signal requested.
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in
            ///        geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
            /// @return The value in SI units of the signal.
            virtual double read_signal(const std::string &signal_name,
                                       int domain_type,
                                       int domain_idx) = 0;
            /// @brief Interpret the setting and write setting to the
            ///        platform.  Does not modify the values stored by
            ///        calling adjust().
            ///
            /// @param [in] control_name Name of the control requested.
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in
            ///        geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
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
            /// @brief Returns a function appropriate for aggregating
            ///        multiple values of the given signal into a
            ///        single value.
            ///
            /// @param [in] signal_name Name of the signal.
            ///
            /// @return A function from vector<double> to double.
            virtual std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const = 0;
            /// @brief Returns a function that can be used to convert
            ///        a signal of the given name into a printable
            ///        string.
            ///
            /// @param [in] signal_name Name of the signal.
            ///
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
            ///
            /// @return One of the IOGroup::m_signal_behavior_e enum
            ///         values that identifies the signal behavior
            virtual int signal_behavior(const std::string &signal_name) const = 0;
            /// @brief Save the state of all controls so that any
            ///        subsequent changes made through PlatformIO can
            ///        be undone with a call to the restore_control()
            ///        method.  Each IOGroup that supports controls
            ///        will populate one file in the save directory
            ///        that contains the saved state and name the file
            ///        after the IOGroup name.
            ///
            /// @param [in] save_dir Directory to be populated with
            ///        save files.
            virtual void save_control(const std::string &save_dir) = 0;
            /// @brief Restore all controls to values recorded in
            ///        previous call to the save_control(save_dir)
            ///        method.  The directory provided contains the
            ///        result of the previous saved state.
            ///
            /// @param [in] save_dir Directory populated with save
            ///        files.
            virtual void restore_control(const std::string &save_dir) = 0;
            ///
            virtual void start_batch_server(int client_pid,
                                            const std::vector<geopm_request_s> &signal_config,
                                            const std::vector<geopm_request_s> &control_config,
                                            int &server_pid,
                                            std::string &server_key) = 0;
            virtual void stop_batch_server(int server_pid) = 0;

            /// @param [in] value Check if the given parameter is a valid value.
            ///
            /// @return true if the value is valid, false if the value is invalid.
            static bool is_valid_value(double value);
    };

    PlatformIO &platform_io(void);
}

#endif
