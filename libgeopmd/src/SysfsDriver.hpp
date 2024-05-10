/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSFSDRIVER_HPP_INCLUDE
#define SYSFSDRIVER_HPP_INCLUDE

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
            /// @brief Arbitrary buffer size
            ///
            /// We're generally looking at integer values much shorter
            /// than 100 digits in length. The IOGroup performs string
            /// truncation checks in case that ever changes.
            static constexpr size_t M_IO_BUFFER_SIZE = 128;
            /// @brief The properties about a signal or control
            struct properties_s {
                std::string name; // The full low level PlatformIO name
                bool is_writable; // True if this is a control property
                std::string attribute; // sysfs attribute name
                std::string description; // Long description for documentation
                double scaling_factor; // SI unit conversion factor
                int units; // IOGroup::m_units_e
                std::function<double(const std::vector<double> &)> aggregation_function;
                int behavior; // IOGroup::m_signal_behavior_e
                std::function<std::string(double)> format_function;
                std::string alias; // Either empty string or name of high level alias
            };
            SysfsDriver() = default;
            virtual ~SysfsDriver() = default;
            /// @brief Get the PlatformTopo domain type for an named attribute
            ///
            /// @param [in] name The name of the signal or control
            ///
            /// @return geopm_domain_e domain type
            virtual int domain_type(const std::string &name) const = 0;
            /// @brief Get the path to the sysfs entry for signal.
            ///
            /// @param [in] name The name of the signal or control
            ///
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            ///
            /// @return File path to the sysfs entry to be read.
            virtual std::string attribute_path(const std::string &name,
                                               int domain_idx) = 0;
            /// @brief Get function to convert contents of sysfs file into signal
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
            virtual std::function<double(const std::string&)> signal_parse(const std::string &signal_name) const = 0;
            /// @brief Get a function to convert a control into a sysfs string
            ///
            /// Converts from the SI unit control into the text
            /// representation required by the device driver.
            ///
            /// @param [in] signal_name The name of the signal.
            ///
            /// @return Function returning string content to be written to sysfs file.
            virtual std::function<std::string(double)> control_gen(const std::string &control_name) const = 0;
            /// Name of the Linux kernel device driver
            ///
            /// @return Name of device driver
            virtual std::string driver(void) const = 0;
            /// Query the meta data about a signal or control
            virtual std::map<std::string, SysfsDriver::properties_s> properties(void) const = 0;
            static std::map<std::string, SysfsDriver::properties_s> parse_properties_json(const std::string &iogroup_name, const std::string &properties_json);
    };
}

#endif
