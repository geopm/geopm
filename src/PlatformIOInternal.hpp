/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#ifndef PLATFORMIOINTERNAL_HPP_INCLUDE
#define PLATFORMIOINTERNAL_HPP_INCLUDE

#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>
#include <tuple>

#include "PlatformIO.hpp"

namespace geopm
{
    class IOGroup;
    class CombinedSignal;
    class IPlatformTopo;

    class PlatformIO : public IPlatformIO
    {
        public:
            /// @brief Constructor for the PlatformIO class.
            PlatformIO();
            PlatformIO(std::list<std::shared_ptr<IOGroup> > iogroup_list,
                       IPlatformTopo &topo);
            PlatformIO(const PlatformIO &other) = delete;
            PlatformIO & operator=(const PlatformIO&) = delete;
            /// @brief Virtual destructor for the PlatformIO class.
            virtual ~PlatformIO() = default;
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
            /// @brief Sample a combined signal using the saved function and operands.
            double sample_combined(int signal_idx);
            bool m_is_active;
            IPlatformTopo &m_platform_topo;
            std::list<std::shared_ptr<IOGroup> > m_iogroup_list;
            std::vector<std::pair<IOGroup *, int> > m_active_signal;
            std::vector<std::pair<IOGroup *, int> > m_active_control;
            std::map<std::tuple<std::string, int, int>, int> m_existing_signal;
            std::map<std::tuple<std::string, int, int>, int> m_existing_control;
            std::map<int, std::pair<std::vector<int>,
                                    std::unique_ptr<CombinedSignal> > > m_combined_signal;
            std::map<int, std::vector<int> > m_combined_control;
            bool m_do_restore;
    };
}

#endif
