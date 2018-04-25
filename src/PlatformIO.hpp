/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace geopm
{
    class IOGroup;

    /// @brief Class which is a collection of all valid control and
    /// signal objects for a platform
    class IPlatformIO
    {
        public:
            IPlatformIO() = default;
            virtual ~IPlatformIO() = default;
            /// @brief Registers an IOGroup with the PlatformIO so
            ///        that its signals and controls are available
            ///        through the PlatformIO interface.
            /// @param [in] iogroup Shared pointer to the IOGroup.
            virtual void register_iogroup(std::shared_ptr<IOGroup> iogroup) = 0;
            /// @brief Query the domain for a named signal.
            /// @param [in] signal_name The name of the signal.
            /// @return One of the PlatformTopo::m_domain_e values
            ///         signifying the granularity at which the signal
            ///         is measured.  Will return M_DOMAIN_INVALID if
            ///         the signal name is not supported.
            virtual int signal_domain_type(const std::string &signal_name) const = 0;
            /// @brief Query the domain for a named control.
            /// @param [in] signal_name The name of the signal.
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
            ///         on the platform.
            virtual int push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Push a previously registered signal to be
            ///        accumulated as a new per-region version of the
            ///        signal. Note that unlike other signals this is
            ///        a total accumulated per region by subtracting
            ///        the value of the signal at the region exit from
            ///        the region entry.  Region entry and exit are
            ///        not exact and are determined by the value of
            ///        the REGION_ID# signal at the time of
            ///        read_batch().  This aggregation should not be
            ///        used for signals that are not monotonically
            ///        increasing, such as frequency.
            /// @param [in] signal_idx Index returned by a previous
            ///        call to push_signal.  If the signal_idx is
            ///        not a previously registered signal, this
            ///        function throws.
            /// @param [in] domain_type Domain type over which the
            ///        region ID should be sampled. This must match
            ///        the domain type of the signal.
            /// @param [in] domain_idx Domain over which the region ID
            ///        should be sampled. This must match the domain
            ///        index of the signal.
            /// @return Index of signal when sample() method is called.
            virtual void push_region_signal_total(int signal_idx,
                                                  int domain_type,
                                                  int domain_idx) = 0;
            virtual int push_combined_signal(const std::string &signal_name,
                                             int domain_type,
                                             int domain_idx,
                                             const std::vector<int> &sub_signal_idx) = 0;
            /// @brief Push a control onto the end of the vector that
            ///        can be adjusted.
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of the control if the requested control
            ///         is valid, otherwise throws.
            virtual int push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx) = 0;
            /// @brief Return number of signals that have been pushed.
            virtual int num_signal(void) const = 0;
            /// @brief Return number of controls that have been pushed.
            virtual int num_control(void) const = 0;
            /// @brief Sample a single signal that has been pushed on
            ///        to the signal stack.  Must be called after a call
            ///        to read_signal(void) method which updates the state
            ///        of all signals.
            /// @param [in] signal_idx index returned by a previous call
            ///        to the push_signal() method.
            /// @return Signal value measured from the platform in SI units.
            virtual double sample(int signal_idx) = 0;
            /// @brief Sample a signal that has been pushed to
            ///        accumlate as per-region values.  Note that
            ///        unlike other signals this is a total
            ///        accumulated per region by subtracting the value
            ///        of the signal at the region exit from the
            ///        region entry.  Region entry and exit are not
            ///        exact and are determined by the value of the
            ///        REGION_ID# signal at the time of read_batch().
            /// @param [in] signal_idx Index returned by a previous call to
            ///        push_signal.  It must also have been passed to
            ///        push_region_signal_total to start the accumlation.
            /// @param [in] region_id The region ID to look up data for.
            /// @return Total accumulated value for the signal for one region.
            virtual double sample_region_total(int signal_idx, uint64_t region_id) = 0;
            /// @brief Adjust a single control that has been pushed on
            ///        to the control stack.  This control will not
            ///        take effect until the next call to
            ///        write_control(void).
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
            /// @brief Returns a function appropriate for aggregating
            ///        multiple values of the given signal into a
            ///        single value.
            /// @param [in] signal_name Name of the signal.
            /// @return A function from vector<double> to double.
            virtual std::function<double(const std::vector<double> &)> agg_function(std::string signal_name) = 0;
            /// @brief Returns the sum of the input operands.
            static double agg_sum(const std::vector<double> &operand);
            /// @brief Returns the average of the input operands.
            static double agg_average(const std::vector<double> &operand);
            /// @brief Returns the median of the input operands.
            static double agg_median(const std::vector<double> &operand);
            /// @brief Returns the output of AND over all the operands
            ///        where 0.0 is false and all other values are
            ///        true.
            static double agg_and(const std::vector<double> &operand);
            /// @brief Returns the output of OR over all the operands
            ///        where 0.0 is false and all other values are
            ///        true.
            static double agg_or(const std::vector<double> &operand);
            /// @brief Returns the minimum value from the input
            ///        operands.
            static double agg_min(const std::vector<double> &operand);
            /// @brief Returns the maximum value from the input
            ///        operands.
            static double agg_max(const std::vector<double> &operand);
            /// @brief Returns the standard deviation of the input
            ///        operands.
            static double agg_stddev(const std::vector<double> &operand);
            /// @brief If all operands are the same, returns the
            ///        common value.  Otherwise, returns
            ///        GEOPM_REGION_ID_UNMARKED.  This is intended for
            ///        situations where all ranks in a domain must be
            ///        in the same region to exert control for that
            ///        region.
            static double agg_region_id(const std::vector<double> &operand);
            /// @brief Structure describing the values required to
            ///        push a signal or control.
            struct m_request_s {
                std::string name;
                int domain_type;
                int domain_idx;
            };
    };

    IPlatformIO &platform_io(void);
}

#endif
