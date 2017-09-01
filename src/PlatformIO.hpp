/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <string>
#include <vector>

#include "geopm_time.h"

namespace geopm
{
    /// @brief Abstract base class describing a signal provided by a
    /// platform that can be sampled.
    class ISignal
    {
        public:
            ISignal() {}
            virtual ~ISignal() {}
            /// @brief Get the signal parameter name.
            /// @param [out] signal_name The name of the feature
            ///        being measured.
            virtual void name(std::string &signal_name) const = 0;
            /// @brief Get the type of the domain under measurement.
            /// @return One of the values from the geopm_domain_type_e
            ///         enum described in PlatformTopology.hpp.
            virtual int domain_type(void) const = 0;
            /// @brief Get the index of the domain under measurement.
            /// @return The index of the domain within the set of
            ///        domains of the same type on the platform.
            virtual int domain_idx(void) const = 0;
            /// @brief Get the value of the signal.
            /// @return The value of the parameter measured in SI
            ///         units.
            virtual double sample(void) const = 0;
    };

    /// @brief Abstract base class describing a control provided by a
    /// platform that can be adjusted.
    class IControl
    {
        public:
            IControl() {}
            virtual ~IControl() {}
            /// @brief Get the control parameter name.
            /// @param [out] control_name The name of the feature
            ///        under control.
            virtual void name(std::string &control_name) const = 0;
            /// @brief Get the type of the domain under control.
            /// @return One of the values from the geopm_domain_type_e
            ///         enum described in PlatformTopology.hpp.
            virtual int domain_type(void) const = 0;
            /// @brief Get the index of the domain under control.
            /// @return The index of the domain within the set of
            ///        domains of the same type on the platform.
            virtual int domain_idx(void) const = 0;
            /// @brief Set the value for the control.
            /// @param [in] setting value in SI units of the parameter
            ///        controlled by the object.
            virtual void adjust(double setting) = 0;
    };

    /// @brief Class which is a collection of all valid control and
    /// signal objects for a platform
    class IPlatformIO
    {
        public:
            IPlatformIO() {}
            virtual ~IPlatformIO() {}
            /// @brief Query for a named signal object for a specified
            ///        domain.
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_type_e enum described in
            ///        PlatformTopology.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Pointer to an ISignal object if the requested
            ///         signal is valid, otherwise returns NULL.
            virtual ISignal *signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Query for a named control object for a
            ///        specified domain.
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        geopm_domain_type_e enum described in
            ///        PlatformTopology.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Pointer to an IControl object if the requested
            ///         control is valid, otherwise returns NULL.
            virtual IControl *control(const std::string &control_name,
                                      int domain_type,
                                      int domain_idx) = 0;
            /// @brief Specify the ordering of signals and controls
            ///        for the sample() and adjust() methods of this
            ///        class.
            /// @param [in] signal Vector of signals that will be
            ///        measured when the sample() method is called.  The
            ///        order of these signals determines the order of
            ///        the values in the signal vector passed to the
            ///        sample() method.
            /// @param [in] control Vector of controls that will be
            ///        set when the adjust() method is called.  The
            ///        order of these controls determines the order of
            ///        the values in the setting vector passed to the
            ///        adjust() method
            virtual void config(const std::vector<ISignal *> &signal,
                                const std::vector<IControl *> &control) = 0;
            /// @brief Measure the signals specified by a previous
            ///        call to the config() method.
            /// @param [out] signal Vector of signal values measured
            ///        from the platform in SI units.  The order of
            ///        these signals is determined by the previous
            ///        call to the config() method.
            /// @param [out] time Time stamp marking when the signals
            ///        were recorded.
            virtual void sample(std::vector<double> &signal,
                                struct geopm_time_s &time) = 0;
            /// @brief Set values of controls specified by a previous
            ///        call to the config() method.
            /// @param [in] setting Vector of control parameter values
            ///        in SI units.  The order of these controls is
            ///        determined by the previous call to the config()
            ///        method.
            /// @brief [out] time Time stamp marking when the controls
            ///        were set.
            virtual void adjust(const std::vector<double> &setting,
                                struct geopm_time_s &time) = 0;
    };


    class PlatformIO : public IPlatformIO
    {
        public:
            /// @brief Constructor for the PlatformIO class.
            /// @param [in] signal Vector of all signals that are
            ///        valid on the platform.
            /// @param [in] signal Vector of all controls that are
            ///        valid on the platform.
            PlatformIO(const std::vector<ISignal *>signal,
                       const std::vector<IControl *>control);
            /// @brief Virtual destructor for the PlatformIO class.
            virtual ~PlatformIO();
            ISignal *signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx);
            IControl *control(const std::string &control_name,
                              int domain_type,
                              int domain_idx);
            void config(const std::vector<ISignal *> &signal,
                        const std::vector<IControl *> &control);
            void sample(std::vector<double> &signal,
                        struct geopm_time_s &time);
            void adjust(const std::vector<double> &setting,
                        struct geopm_time_s &time);
    };
}

#endif
