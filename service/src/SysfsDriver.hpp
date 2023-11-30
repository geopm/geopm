/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSFSDRIVER_HPP_INCLUDE
#define SYSFSDRIVER_HPP_INCLUDE

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace geopm
{
    /// @brief Class used to implement the SysfsIOGroup base class
    ///
    /// This virtual interface can be adapted for each Linux device
    /// driver.  A concrete implementation can be used to construct a
    /// SysfsIOGroup object.
    class SysfsDriver
    {
        public:
            /// @brief The properties about a signal or control
            struct properties_s {
                std::string name; // The full low level PlatformIO name
                bool is_writable; // Is control property?
                std::string attribute; // sysfs attribute name
                std::string description; // Long description for documentation
                double scaling_factor; // SI unit conversion factor
                int domain; // native domain geopm_domain_e
                int units; // IOGroup::m_units_e
                std::string aggregation; // Agg class name
                int behavior; // IOGroup::m_signal_behavior_e
                std::string format; // format function name
                std::string alias; // Either empty string or name of high level alias
            };
            SysfsDriver() = default;
            virtual ~SysfsDriver() = default;
            /// @brief Get supported signal names.
            ///
            /// @return Vector of all supported signals
            virtual std::vector<std::string> signal_names(void) const = 0;
            /// @brief Get supported control names.
            ///
            /// @return Vector of all supported controls
            virtual std::vector<std::string> control_names(void) const = 0;
            /// @brief Get the path to the sysfs entry for signal.
            ///
            /// @param [in] signal_name The name of the signal
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
            /// @return File path to the sysfs entry to be read.
            virtual std::string signal_path(const std::string &signal_name,
                                            int domain_type,
                                            int domain_idx) = 0;
            /// @brief Get the path to the sysfs entry for control.
            ///
            /// @param [in] control_name The name of the control.
            ///
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_e enum described in geopm_topo.h
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
            /// @return File path to the sysfs entry to be written.
            virtual std::string control_path(const std::string &control_name,
                                             int domain_type,
                                             int domain_idx) const = 0;
            /// @brief Convert contents of sysfs file into signal
            ///
            /// This parsing includes the conversion of the numerical
            /// data into SI units.
            ///
            /// @param [in] signal_name The name of the signal.
            ///
            /// @param [in] content The string content read from the
            ///        sysfs file.
            ///
            /// @return The parsed signal value in SI units.
            virtual double signal_parse(const std::string &signal_name,
                                        const std::string &content) const = 0;
            /// @brief Convert a control into a sysfs string
            ///
            /// Converts from the SI unit control into the text
            /// representation required by the device driver.
            ///
            /// @param [in] control_name The name of the control.
            ///
            /// @param [in] setting The control setting in SI units.
            ///
            /// @return String content to be written to sysfs file.
            virtual std::string control_gen(const std::string &control_name,
                                            double setting) const = 0;
            /// @brief Convert contents of sysfs file into signal
            ///
            /// This parsing includes the conversion of the numerical
            /// data into SI units.
            ///
            /// @param [in] properties_id The unique identifier of the signal.
            ///
            /// @param [in] content The string content read from the
            ///        sysfs file.
            ///
            /// @return The parsed signal value in SI units.
            virtual double signal_parse(int properties_id,
                                        const std::string &content) const = 0;
            /// @brief Convert a control into a sysfs string
            ///
            /// Converts from the SI unit control into the text
            /// representation required by the device driver.
            ///
            /// @param [in] properties_id The unique identifier of the control.
            ///
            /// @param [in] setting The control setting in SI units.
            ///
            /// @return String content to be written to sysfs file.
            virtual std::string control_gen(int properties_id,
                                            double setting) const = 0;
            /// Name of the Linux kernel device driver
            ///
            /// @return Name of device driver
            virtual std::string driver(void) const = 0;
            /// Query the meta data about a signal or control
            virtual struct properties_s properties(const std::string &name) const = 0;
            /// Get all of the meta data in JSON format.
            virtual std::string properties_json(void) const = 0;
            static std::map<std::string, SysfsDriver::properties_s> parse_properties_json(const std::string &properties_json);
    };
}

#endif
