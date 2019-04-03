/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
            /// @brief Checks that the minimum and maximum frequency
            ///        are within range for the platform.  If not,
            ///        they will be clamped at the min and max for the
            ///        platform.
            /// @param [inout] freq_min Minimum frequency to attempt
            ///        to set, and resulting valid minimum.
            /// @param [inout] freq_max Maximum frequency to attempt
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
