/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef IOGROUP_HPP_INCLUDE
#define IOGROUP_HPP_INCLUDE

#include <string>
#include <vector>
#include <set>
#include <functional>

#include "PluginFactory.hpp"

namespace geopm
{
    class IOGroup
    {
        public:
            enum m_units_e {
                M_UNITS_NONE,
                M_UNITS_SECONDS,
                M_UNITS_HERTZ,
                M_UNITS_WATTS,
                M_UNITS_JOULES,
                M_UNITS_CELSIUS,
                M_NUM_UNITS
            };

            /// @brief Description of the runtime behavior of a signal
            enum m_signal_behavior_e {
                /// signals that have a contant value
                M_SIGNAL_BEHAVIOR_CONSTANT,
                /// signals that increase monotonically
                M_SIGNAL_BEHAVIOR_MONOTONE,
                /// signals that vary up and down over time
                M_SIGNAL_BEHAVIOR_VARIABLE,
                /// signals that should not be summarized over time
                M_SIGNAL_BEHAVIOR_LABEL,
                M_NUM_SIGNAL_BEHAVIOR
            };

            IOGroup() = default;
            virtual ~IOGroup() = default;
            static std::vector<std::string> iogroup_names(void);
            static std::unique_ptr<IOGroup> make_unique(const std::string &iogroup_name);
            /// @brief Returns the names of all signals provided by
            ///        the IOGroup.
            virtual std::set<std::string> signal_names(void) const = 0;
            /// @brief Returns the names of all controls provided by
            ///        the IOGroup.
            virtual std::set<std::string> control_names(void) const = 0;
            /// @brief Test if signal_name refers to a signal
            ///        supported by the group.
            /// @param [in] signal_name Name of signal to test.
            /// @return True if signal is supported, false otherwise.
            virtual bool is_valid_signal(const std::string &signal_name) const = 0;
            /// @brief Test if control_name refers to a control
            ///        supported by the group.
            /// @param [in] control_name Name of control to test.
            /// @return True if control is supported, false otherwise.
            virtual bool is_valid_control(const std::string &control_name) const = 0;
            /// @brief Query the domain for a named signal.
            /// @param [in] signal_name Name of the signal to query.
            /// @return One of the PlatformTopo::m_domain_e enum values.
            virtual int signal_domain_type(const std::string &signal_name) const = 0;
            /// @brief Query the domain for a named control.
            /// @param [in] control_name Name of the control to query.
            /// @return One of the PlatformTopo::m_domain_e enum values.
            virtual int control_domain_type(const std::string &control_name) const = 0;
            /// @brief Add a signal to the list of signals that is
            ///        read by read_batch() and sampled by sample().
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        PlatformTopo::m_domain_e enum described in
            ///        PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of signal when sample() method is called.
            virtual int push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Add a control to the list of controls that is
            ///        written by write_batch() and configured with
            ///        adjust().
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        PlatformTopo::m_domain_e enum described in
            ///        PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of control when adjust() method is called.
            virtual int push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx) = 0;
            /// @brief Read all pushed signals from the platform so
            ///        that the next call to sample() will reflect the
            ///        updated data.
            virtual void read_batch(void) = 0;
            /// @brief Write all of the pushed controls so that values
            ///        previously given to adjust() are written to the
            ///        platform.
            virtual void write_batch(void) = 0;
            /// @brief Retrieve signal value from data read by last
            ///        call to read_batch() for a particular signal
            ///        previously pushed with push_signal().
            /// @param [in] sample_idx The index returned by previous
            ///        call to push_signal().
            /// @return Value of signal in SI units.
            virtual double sample(int sample_idx) = 0;
            /// @brief Adjust a setting for a particular control that
            ///        was previously pushed with push_control(). This
            ///        adjustment will be written to the platform on
            ///        the next call to write_batch().
            /// @param [in] control_idx The index returned by previous
            ///        call to push_control().
            /// @param [in] setting Value of the control in SI units.
            virtual void adjust(int control_idx,
                                double setting) = 0;
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
            /// @brief Save the state of all controls so that any
            ///        subsequent changes made through the IOGroup
            ///        can be undone with a call to the restore()
            ///        method.
            virtual void save_control(void) = 0;
            /// @brief Restore all controls to values recorded in
            ///        previous call to the save() method.
            virtual void restore_control(void) = 0;
            /// @brief Return a function that should be used when aggregating
            ///        the given signal.
            /// @param [in] signal_name Name of the signal.
            virtual std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const = 0;
            /// @brief Returns a function that can be used to convert
            ///        a signal of the given name into a printable
            ///        string.  May be one of the IOGroup::format_*()
            ///        static methods, or another function.
            virtual std::function<std::string(double)> format_function(const std::string &signal_name) const;
            /// @brief Returns a description of the signal.  This
            ///        string can be used by tools to generate help
            ///        text for users of the IOGroup.
            virtual std::string signal_description(const std::string &signal_name) const = 0;
            /// @brief Returns a description of the control.  This
            ///        string can be used by tools to generate help
            ///        text for users of the IOGroup.
            virtual std::string control_description(const std::string &control_name) const = 0;
            /// @brief Returns a hint about how a signal will change
            ///        as a function of time.
            ///
            /// This can be used when generating reports to decide how
            /// to summarize a signal's value for the entire
            /// application run.
            ///
            /// @param [in] signal_name Name of the signal.
            virtual int signal_behavior(const std::string &signal_name) const = 0;

            /// @brief Convert a string to the corresponding m_units_e value
            static m_units_e string_to_units(const std::string &str);
            /// @brief Convert the m_units_e value to the corresponding string.
            static std::string units_to_string(int);
            /// @brief Convert a string to the corresponding m_signal_behavior_e value
            static m_signal_behavior_e string_to_behavior(const std::string &str);

            static const std::string M_PLUGIN_PREFIX;

        private:
            static const std::string M_UNITS[];
            static const std::string M_BEHAVIORS[];
            static const std::map<std::string, m_units_e> M_UNITS_STRING;
            static const std::map<std::string, m_signal_behavior_e> M_BEHAVIOR_STRING;
    };

    class IOGroupFactory : public PluginFactory<IOGroup>
    {
        public:
            IOGroupFactory();
            virtual ~IOGroupFactory() = default;
    };

    IOGroupFactory &iogroup_factory(void);
}

#endif
