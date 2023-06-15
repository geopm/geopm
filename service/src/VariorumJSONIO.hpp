/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef VARIORUMIOGROUP_HPP_INCLUDE
#define VARIORUMIOGROUP_HPP_INCLUDE

#include <functional>
#include <map>

#include "geopm/IOGroup.hpp"
#include "geopm_time.h"

namespace geopm
{
    /// @brief IOGroup that wraps interfaces to Compute Node Linux.
    ///
    /// @details The VariorumIOGroup provides board-level energy counters from Compute Node Linux
    ///          as signals.  These values are obtained through the proc(5)
    class VariorumIOGroup : public IOGroup
    {
        public:
            VariorumIOGroup();
            VariorumIOGroup(const std::string &pm_counters_path);
            virtual ~VariorumIOGroup() = default;
            /// @return the list of signal names provided by this IOGroup.
            std::set<std::string> signal_names(void) const override;
            /// @return empty set; this IOGroup does not provide any controls.
            std::set<std::string> control_names(void) const override;
            /// @return whether the given signal_name is supported by the
            ///         VariorumIOGroup for the current platform.
            bool is_valid_signal(const std::string &signal_name) const override;
            /// @return false; this IOGroup does not provide any controls.
            bool is_valid_control(const std::string &control_name) const override;
            /// @return GEOPM_DOMAIN_BOARD; If the signal_name is valid for this IOGroup,
            ///         GEOPM_DOMAIN_INVALID otherwise if the signal_name is invalid for this IOGroup.
            int signal_domain_type(const std::string &signal_name) const override;
            /// @return GEOPM_DOMAIN_INVALID; this IOGroup does not provide any controls.
            int control_domain_type(const std::string &control_name) const override;
            /// @param signal_name Adds the signal specified to the list of signals to be read during read_batch()
            ///
            /// @param domain_type This must be == GEOPM_DOMAIN_BOARD
            ///
            /// @param domain_idx is ignored
            ///
            /// @throws geopm::Exception
            ///         if signal_name is not valid
            ///         if domain_type is not GEOPM_DOMAIN_BOARD
            int push_signal(const std::string &signal_name, int domain_type,
                            int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the VariorumIOGroup
            int push_control(const std::string &control_name, int domain_type,
                             int domain_idx) override;
            /// @brief Read all pushed signals from the platform so that the next call to
            ///        sample() will reflect the updated data.
            ///
            /// @details The intention is that read_batch() will read the all of the
            ///          IOGroup's signals into memory once per call.
            void read_batch(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void write_batch(void) override;
            /// @param batch_idx Specifies a signal_idx returned from push_signal()
            ///
            /// @return the value of the signal specified by a signal_idx returned from push_signal().
            ///
            /// @details The value will have been updated by the most recent call to read_batch().
            double sample(int batch_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the VariorumIOGroup
            void adjust(int batch_idx, double setting) override;
            /// @param signal_name Specifies the name of the signal whose value you want to get.
            ///
            /// @param domain_type This must be == GEOPM_DOMAIN_BOARD
            ///
            /// @param domain_idx is ignored
            ///
            /// @throws geopm::Exception
            ///         if signal_name is not valid
            ///         if domain_type is not GEOPM_DOMAIN_BOARD
            ///
            /// @return the stored value for the given signal_name.
            double read_signal(const std::string &signal_name, int domain_type,
                               int domain_idx) override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the VariorumIOGroup
            void write_control(const std::string &control_name, int domain_type,
                               int domain_idx, double setting) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void save_control(void) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            void restore_control(void) override;
            /// @param signal_name Specifies the name of the signal to be used for aggregation.
            ///
            /// @return  a function that should be used when aggregating the given signal.
            ///
            /// @see geopm::Agg
            std::function<double(const std::vector<double> &)>
                agg_function(const std::string &signal_name) const override;
            /// @return  a function that should be used when formatting the given signal.
            ///
            /// @see geopm::Agg
            std::function<std::string(double)>
                format_function(const std::string &signal_name) const override;
            /// @return a string description for signal_name, if defined.
            std::string signal_description(const std::string &signal_name) const override;
            /// @brief Should not be called; this IOGroup does not provide any controls.
            ///
            /// @throws geopm::Exception there are no controls supported by the VariorumIOGroup
            std::string control_description(const std::string &control_name) const override;
            /// @return one of the IOGroup::signal_behavior_e values which
            ///         describes about how a signal will change as a function of time.
            ///
            /// @details This can be used when generating reports to decide how to
            ///          summarize a signal's value for the entire application run.
            int signal_behavior(const std::string &signal_name) const override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            ///
            /// @param save_path this argument is ignored
            void save_control(const std::string &save_path) override;
            /// @brief Does nothing; this IOGroup does not provide any controls.
            ///
            /// @param save_path this argument is ignored
            void restore_control(const std::string &save_path) override;
            /// @return the name of the IOGroup
            std::string name(void) const override;
            /// @return the name of the plugin to use when this plugin is
            ///         registered with the IOGroup factory
            ///
            /// @see geopm::PluginFactory
            static std::string plugin_name(void);
            /// @return a pointer to a new VariorumIOGroup object
            ///
            /// @see geopm::PluginFactory
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            void register_signal_alias(const std::string &alias_name, const std::string &signal_name);

            struct m_signal_info_s {
                std::string m_description;
//                std::function<double(const std::vector<double> &)> m_agg_function;
//                std::function<std::string(double)> m_format_function;
                std::function<double()> m_read_function;
                bool m_do_read;
                double m_value;
                int m_units;
//                int m_behavior;
            };
            std::map<std::string, m_signal_info_s> m_signal_available;

            double read_time(const std::string &freshness_path) const;

            geopm_time_s m_time_zero;
            double m_initial_freshness;
            double m_sample_rate;
    };
}

#endif
