/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DEBUGIOGROUP_HPP_INCLUDE
#define DEBUGIOGROUP_HPP_INCLUDE

#include <cstdint>

#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <set>

#include "geopm/IOGroup.hpp"

namespace geopm
{
    class PlatformTopo;

    /// @brief IOGroup that Agents can use to expose internal values.
    class DebugIOGroup : public IOGroup
    {
        public:
            /// @brief Constructor; should be called in the Agent's
            ///        constructor.  value_cache is created and
            ///        updated by the Agent, but the lifetime of the
            ///        IOGroup may be longer than the Agent.
            DebugIOGroup(const PlatformTopo &topo,
                         std::shared_ptr<std::vector<double> > value_cache);
            virtual ~DebugIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;
            void save_control(const std::string &save_path) override;
            void restore_control(const std::string &save_path) override;
            std::string name(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

            /// @brief Set up a signal name and base domain to map to one or
            ///        more underlying values.  One signal will be added for
            ///        each index in the domain.
            void register_signal(const std::string &name, int domain_type,
                                 int signal_behavior);
        private:
            struct m_signal_info_s {
                int domain_type;
                int behavior;
            };

            const PlatformTopo &m_topo;
            const std::shared_ptr<std::vector<double> > m_value_cache;
            size_t m_num_reg_signals;
            /// map key is signal_name,domain_idx
            std::map<std::pair<std::string, int>, int> m_signal_idx;
            std::map<std::string, m_signal_info_s> m_signal_info;
            std::set<std::string> m_signal_name;
    };
}

#endif
