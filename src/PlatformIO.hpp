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

#ifndef PLATFORMIO_HPP_INCLUDE
#define PLATFORMIO_HPP_INCLUDE

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <set>
#include <list>
#include <map>

namespace geopm
{
    class IOGroup;

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
            /// @param [in] iogroup Shared pointer to the IOGroup.
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
            /// @param [in] signal_name The name of the signal.
            /// @return One of the PlatformTopo::m_domain_e values
            ///         signifying the granularity at which the signal
            ///         is measured.  Will return M_DOMAIN_INVALID if
            ///         the signal name is not supported.
            virtual int signal_domain_type(const std::string &signal_name) const = 0;
            /// @brief Query the domain for a named control.
            /// @param [in] control_name The name of the control.
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
            ///         on the platform.  Returned signal index will be
            ///         repeated for each unique tuple of push_signal
            ///         input parameters.
            virtual int push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx) = 0;
            /// @brief Push a control onto the end of the vector that
            ///        can be adjusted.
            /// @param [in] control_name Name of the control requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @return Index of the control if the requested control
            ///         is valid, otherwise throws.  Returned control index
            ///         will be repeated for each unique tuple of the push_control
            ///         input parameters.
            virtual int push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx) = 0;
            /// @brief Return number of signals that have been pushed.
            virtual int num_signal(void) const = 0;
            /// @brief Return number of controls that have been pushed.
            virtual int num_control(void) const = 0;
            /// @brief Sample a single signal that has been pushed on
            ///        to the signal stack.  Must be called after a call
            ///        to read_batch(void) method which updates the state
            ///        of all signals.
            /// @param [in] signal_idx index returned by a previous call
            ///        to the push_signal() method.
            /// @return Signal value measured from the platform in SI units.
            virtual double sample(int signal_idx) = 0;
            /// @brief Adjust a single control that has been pushed on
            ///        to the control stack.  This control will not
            ///        take effect until the next call to
            ///        write_batch(void).
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
            /// @param [in] signal_name Name of the signal.
            /// @return A function from vector<double> to double.
            virtual std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const = 0;
            /// @brief Returns a description of the signal.  This
            ///        string can be used by tools to generate help
            ///        text for users of PlatformIO.
            virtual std::string signal_description(const std::string &signal_name) const = 0;
            /// @brief Returns a description of the control.  This
            ///        string can be used by tools to generate help
            ///        text for users of PlatformIO.
            virtual std::string control_description(const std::string &control_name) const = 0;
            /// @brief Structure describing the values required to
            ///        push a signal or control.
            struct m_request_s {
                std::string name;
                int domain_type;
                int domain_idx;
            };
    };

    PlatformIO &platform_io(void);

    class IOGroup;
    class CombinedSignal;
    class PlatformTopo;

    class PlatformIOImp : public PlatformIO
    {
        public:
            PlatformIOImp();
            PlatformIOImp(std::list<std::shared_ptr<IOGroup> > iogroup_list,
                          PlatformTopo &topo);
            PlatformIOImp(const PlatformIOImp &other) = delete;
            PlatformIOImp & operator=(const PlatformIOImp&) = delete;
            virtual ~PlatformIOImp() = default;
            void register_iogroup(std::shared_ptr<IOGroup> iogroup) override;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx) override;
            int push_control(const std::string &control_name,
                             int domain_type,
                             int domain_idx) override;
            int num_signal(void) const override;
            int num_control(void) const override;
            double sample(int signal_idx) override;
            void adjust(int control_idx, double setting) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double read_signal(const std::string &signal_name,
                               int domain_type,
                               int domain_idx) override;
            void write_control(const std::string &control_name,
                               int domain_type,
                               int domain_idx,
                               double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
        private:
            /// @brief Push a signal that aggregates values sampled
            ///        from other signals.  The aggregation function
            ///        used is determined by a call to agg_function()
            ///        with the given signal name.
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in PlatformTopo.hpp.
            /// @param [in] domain_idx The index of the domain within
            ///        the set of domains of the same type on the
            ///        platform.
            /// @param [in] sub_signal_idx Vector of previously pushed
            ///        signals whose values will be used to generate
            ///        the combined signal.
            /// @return Index of signal when sample() method is called
            ///         or throws if the signal is not valid
            ///         on the platform.
            int push_combined_signal(const std::string &signal_name,
                                     int domain_type,
                                     int domain_idx,
                                     const std::vector<int> &sub_signal_idx);
            /// @brief Save a high-level signal as a combination of other signals.
            /// @param [in] signal_idx Index a caller can use to refer to this signal.
            /// @param [in] operands Input signal indices to be combined.  These must
            ///             be valid pushed signals registered with PlatformIO.
            /// @param [in] func The function that will combine the signals into
            ///             a single result.
            void register_combined_signal(int signal_idx,
                                          std::vector<int> operands,
                                          std::unique_ptr<CombinedSignal> signal);
            int push_signal_power(const std::string &signal_name,
                                  int domain_type,
                                  int domain_idx);
            int push_signal_temperature(const std::string &signal_name,
                                        int domain_type,
                                        int domain_idx);
            int push_signal_convert_domain(const std::string &signal_name,
                                           int domain_type,
                                           int domain_idx);
            int push_control_convert_domain(const std::string &control_name,
                                            int domain_type,
                                            int domain_idx);
            double read_signal_convert_domain(const std::string &signal_name,
                                              int domain_type,
                                              int domain_idx);
            void write_control_convert_domain(const std::string &control_name,
                                              int domain_type,
                                              int domain_idx,
                                              double setting);
            /// @brief Sample a combined signal using the saved function and operands.
            double sample_combined(int signal_idx);
            /// @brief Look up the IOGroup that provides the given signal.
            std::shared_ptr<IOGroup> find_signal_iogroup(const std::string &signal_name) const;
            /// @brief Look up the IOGroup that provides the given control.
            std::shared_ptr<IOGroup> find_control_iogroup(const std::string &control_name) const;
            bool m_is_active;
            PlatformTopo &m_platform_topo;
            std::list<std::shared_ptr<IOGroup> > m_iogroup_list;
            std::vector<std::pair<std::shared_ptr<IOGroup>, int> > m_active_signal;
            std::vector<std::pair<std::shared_ptr<IOGroup>, int> > m_active_control;
            std::map<std::tuple<std::string, int, int>, int> m_existing_signal;
            std::map<std::tuple<std::string, int, int>, int> m_existing_control;
            std::map<int, std::pair<std::vector<int>,
                                    std::unique_ptr<CombinedSignal> > > m_combined_signal;
            std::map<int, std::vector<int> > m_combined_control;
            bool m_do_restore;
    };
}

#endif
