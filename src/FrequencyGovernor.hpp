/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FREQUENCYGOVERNOR_HPP_INCLUDE
#define FREQUENCYGOVERNOR_HPP_INCLUDE

#include <vector>
#include <memory>

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    class FrequencyGovernor
    {
        public:
            FrequencyGovernor() = default;
            virtual ~FrequencyGovernor() = default;
            /// @brief Registers signals and controls with PlatformIO using the
            ///        default control domain.
            virtual void init_platform_io(void) = 0;
            /// @brief Get the domain type of frequency control on the
            ///        platform.  Users of the FrequencyGovernor can
            ///        use this information to determine the size of
            ///        the vector to pass to adjust_platform().
            /// @return The domain with which frequency will be governed.
            virtual int frequency_domain_type(void) const = 0;
            /// @brief Set the domain type of frequency control that will
            ///        be used in adjust_platform(). Must be called before
            ///        init_platform_io().
            /// @throw Exception the requested domain does not contain the
            ///        frequency control's native domain.
            /// @throw Exception The caller attempted to set the domain type
            ///        after this governor initialized its PlatformIO controls.
            virtual void set_domain_type(int domain_type) = 0;
            /// @brief Write frequency control, may be clamped between
            ///        min and max frequency if request cannot be
            ///        satisfied.
            /// @param [in] frequency_request Desired per domain frequency.
            virtual void adjust_platform(const std::vector<double> &frequency_request) = 0;
            /// @brief Returns true if last call to adjust_platform requires writing
            //         of controls.
            /// @return True if platform adjustments have been made, false otherwise.
            virtual bool do_write_batch(void) const = 0;
            /// @brief Sets min and max package bounds.  The defaults before calling
            ///        this method are the min and max frequency for the platform.
            /// @param [in] freq_min Minimum frequency value for the control domain.
            /// @param [in] freq_max Maximum frequency value for the control domain.
            /// @return Returns true if internal state updated, otherwise false.
            virtual bool set_frequency_bounds(double freq_min, double freq_max) = 0;
            /// @brief Returns the current min frequency used by the governor.
            /// @return Minimum frequency.
            virtual double get_frequency_min() const = 0;
            /// @brief Returns the current max frequency used by the governor.
            /// @return Maximum frequency.
            virtual double get_frequency_max() const = 0;
            /// @brief Returns the frequency step for the platform.
            /// @return Step frequency.
            virtual double get_frequency_step() const = 0;
            /// @brief Returns the number of clamping occurrence count for the platform.
            /// @return Clamp occurrence counter
            virtual int get_clamp_count() const = 0;
            /// @brief Checks that the minimum and maximum frequency
            ///        are within range for the platform.  If not,
            ///        they will be clamped at the min and max for the
            ///        platform.
            /// @param [in,out] freq_min Minimum frequency to attempt
            ///        to set, and resulting valid minimum.
            /// @param [in,out] freq_max Maximum frequency to attempt
            ///        to set, and resulting valid maximum.
            virtual void validate_policy(double &freq_min, double &freq_max) const = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<FrequencyGovernor> make_unique(void);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<FrequencyGovernor> make_shared(void);
    };
}

#endif
