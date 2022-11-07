/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POWERGOVERNOR_HPP_INCLUDE
#define POWERGOVERNOR_HPP_INCLUDE

#include <memory>

namespace geopm
{
    class PowerGovernor
    {
        public:
            PowerGovernor() = default;
            virtual ~PowerGovernor() = default;
            /// @brief Registsters signals and controls with PlatformIO.
            virtual void init_platform_io(void) = 0;
            /// @brief To be called inside of the Agent's
            ///        sample_platform() method to read any values
            ///        required when calling adjust_platform().  This
            ///        method is currently a noop since no signals are
            ///        required for this algorithm.
            virtual void sample_platform(void) = 0;
            /// @brief Calculates metric of DRAM power history, subtracting that value
            ///        from the provided target node power.
            /// @param [in] node_power_request Total expected node power consumption.
            /// @param [out] node_power_actual Achievable node power limit.  Should equal
            //              node_power_request unless clamped by bounds.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual void adjust_platform(double node_power_request, double &node_power_actual) = 0;
            virtual bool do_write_batch(void) const = 0;
            /// @brief Sets min and max package bounds.
            /// @param [in] min_pkg_power Minimum package power.
            /// @param [in] max_pkg_power Maximum package power.
            virtual void set_power_bounds(double min_pkg_power, double max_pkg_power) = 0;
            /// @brief Get the time window for controlling package power.
            /// @return Time window in units of seconds.
            virtual double power_package_time_window(void) const = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<PowerGovernor> make_unique(void);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<PowerGovernor> make_shared(void);
    };
}

#endif
