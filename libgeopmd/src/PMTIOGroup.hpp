/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

#include "geopm/IOGroup.hpp"
#include "geopm_time.h"
#include "RolloverGenerator.hpp"

namespace geopm
{
    class PlatformTopo;

    class PMTIOGroup : public IOGroup
    {
        public:
            PMTIOGroup();
            virtual ~PMTIOGroup();

            // IOGroup overrides
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
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
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;

            // Extended save/restore API required by IOGroup
            void save_control(const std::string &save_path) override;
            void restore_control(const std::string &save_path) override;
            std::string name(void) const override;

            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            struct SignalDef {
                // Base container index for domain index 0
                uint32_t base_container;
                // Bit field within the 64-bit container
                uint8_t lsb;
                uint8_t msb;
                // Domain type for this signal
                int domain_type;
                // Units
                int units;
                // Behavior
                int behavior;
                // Human description
                std::string description;
            };

            struct PushedSignal {
                std::string name;
                int domain_type;
                int domain_idx;
                SignalDef spec;
                double value;
                bool is_composite = false;
                std::vector<std::string> components; // used if composite
                bool is_rate = false;                 // true if derivative of components
                double last_time = NAN;               // last sample time for rate
                double last_value = 0.0;              // last summed value for rate
            };

            // Discovery of a matching PMT telem device
            void discover_telem(void);
            // Locate and parse Intel-PMT XML mapping for discovered guid
            void discover_signals_from_xml(void);
            // Read one 64-bit container value
            uint64_t read_container(uint32_t container_idx) const;
            // Extract bit range [lsb, msb] from a 64-bit value
            static uint64_t extract_bits(uint64_t val, uint8_t lsb, uint8_t msb);
            // Fallback: Build minimal field map if XML not found
            void build_minimal_field_map(void);

            // Get or create a rollover generator for a base signal and CPU index
            std::shared_ptr<RolloverGenerator> rollover_for(const std::string &signal_name,
                                                           int domain_idx);

            // Sysfs resources
            std::string m_telem_path; // path to telem binary file
            std::string m_guid_str;
            uint64_t m_guid_val;
            size_t m_telem_size_bytes;
            int m_fd;

            // Signals supported and their field specs
            std::map<std::string, SignalDef> m_signal_def_by_name;
            // Pushed signals for batch
            std::vector<PushedSignal> m_pushed;

            // Cached topo
            int m_num_cpu;
            geopm_time_s m_time_zero;
            // Sleep duration for direct rate calculation (nanoseconds)
            long m_direct_rate_sleep_ns = 5 * 1000 * 1000; // default 5 ms

            // Per-base-signal, per-CPU rollover generators
            // Only used for base (non-virtual) counters to extend monotone sequences across wraps.
            std::map<std::string, std::vector<std::shared_ptr<RolloverGenerator>>> m_rollover_by_signal;
    };
}
