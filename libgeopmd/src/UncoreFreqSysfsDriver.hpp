/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UNCOREFREQSYSFSDRIVER_HPP_INCLUDE
#define UNCOREFREQSYSFSDRIVER_HPP_INCLUDE

#include "SysfsDriver.hpp"
#include "geopm/IOGroup.hpp"

namespace geopm
{
    /// @brief SysfsDriver implementation for the Linux
    ///        intel_uncore_frequency driver.
    ///
    /// On recent Intel Xeon parts (Granite Rapids, Sierra Forest,
    /// Clearwater Forest, and later) the uncore frequency knobs moved
    /// from MSRs to the TPMI memory-mapped interface and are exposed to
    /// userspace through
    /// /sys/devices/system/cpu/intel_uncore_frequency/.  This driver
    /// maps that sysfs surface onto the generic SysfsIOGroup so that the
    /// existing CPU_UNCORE_FREQUENCY_{MAX,MIN}_CONTROL and
    /// CPU_UNCORE_FREQUENCY_STATUS aliases keep working on those
    /// platforms.  Controls are package-scoped: max/min are read from
    /// and written to the package_*_die_* fan-out directory, while the
    /// current-frequency status is read from a representative uncore*
    /// domain in the package.
    class UncoreFreqSysfsDriver : public SysfsDriver
    {
        public:
            UncoreFreqSysfsDriver();
            UncoreFreqSysfsDriver(const std::string &uncore_directory);
            virtual ~UncoreFreqSysfsDriver() = default;
            int domain_type(const std::string &name) const override;
            std::string attribute_path(const std::string &name, int domain_idx) override;
            std::function<double(const std::string &)> signal_parse(const std::string &signal_name) const override;
            std::function<std::string(double)> control_gen(const std::string &control_name) const override;
            std::string driver(void) const override;
            std::map<std::string, properties_s> properties(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            const std::map<std::string, properties_s> M_PROPERTIES;
            const std::string M_UNCORE_DIRECTORY;
            // Package index -> package_*_die_* directory path (fan-out
            // control directory for max/min).
            std::map<int, std::string> m_control_dir_by_package;
            // Package index -> representative uncore* directory path
            // (source for current_freq_khz status).
            std::map<int, std::string> m_status_dir_by_package;
    };
}

#endif
