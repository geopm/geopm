/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORMIOIMP_HPP_INCLUDE
#define PLATFORMIOIMP_HPP_INCLUDE

#include <list>
#include <map>

#include "geopm/PlatformIO.hpp"
#include "geopm_pio.h"

namespace geopm
{
    class IOGroup;
    class CombinedSignal;
    class PlatformTopo;
    class BatchServer;

    class PlatformIOImp : public PlatformIO
    {
        public:
            PlatformIOImp();
            PlatformIOImp(std::list<std::shared_ptr<IOGroup> > iogroup_list,
                          const PlatformTopo &topo);
            PlatformIOImp(const PlatformIOImp &other) = delete;
            PlatformIOImp &operator=(const PlatformIOImp &other) = delete;
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
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;
            void save_control(const std::string &save_dir) override;
            void restore_control(const std::string &save_dir) override;
            void start_batch_server(int client_pid,
                                    const std::vector<geopm_request_s> &signal_config,
                                    const std::vector<geopm_request_s> &control_config,
                                    int &server_pid,
                                    std::string &server_key) override;
            void stop_batch_server(int server_pid) override;

            int num_signal_pushed(void) const;  // Used for testing only
            int num_control_pushed(void) const; // Used for testing only
        private:
            /// @brief Push a signal that aggregates values sampled
            ///        from other signals.  The aggregation function
            ///        used is determined by a call to agg_function()
            ///        with the given signal name.
            /// @param [in] signal_name Name of the signal requested.
            /// @param [in] domain_type One of the values from the
            ///        m_domain_e enum described in geopm/PlatformTopo.hpp.
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
            std::vector<std::shared_ptr<IOGroup> > find_signal_iogroup(const std::string &signal_name) const;
            /// @brief Look up the IOGroup that provides the given control.
            std::vector<std::shared_ptr<IOGroup> > find_control_iogroup(const std::string &control_name) const;
            bool m_is_active;
            const PlatformTopo &m_platform_topo;
            std::list<std::shared_ptr<IOGroup> > m_iogroup_list;
            std::vector<std::pair<std::shared_ptr<IOGroup>, int> > m_active_signal;
            std::vector<std::pair<std::shared_ptr<IOGroup>, int> > m_active_control;
            std::map<std::tuple<std::string, int, int>, int> m_existing_signal;
            std::map<std::tuple<std::string, int, int>, int> m_existing_control;
            std::map<int, std::pair<std::vector<int>,
                                    std::unique_ptr<CombinedSignal> > > m_combined_signal;
            std::map<int, std::vector<int> > m_combined_control;
            bool m_do_restore;
            std::map<int, std::shared_ptr<BatchServer> > m_batch_server;
            static const std::map<const std::string, const std::string> m_signal_descriptions;
            static const std::map<const std::string, const std::string> m_control_descriptions;
    };
}

#endif
