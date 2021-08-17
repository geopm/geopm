/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "LevelZeroIOGroup.hpp"

#include <cmath>

#include <iostream>
#include <string>
#include <numeric>
#include <sched.h>
#include <errno.h>

#include "IOGroup.hpp"
#include "Signal.hpp"
#include "DerivativeSignal.hpp"
#include "PlatformTopo.hpp"
#include "LevelZeroDevicePool.hpp"
#include "LevelZero.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "Helper.hpp"
#include "geopm_debug.hpp"

namespace geopm
{

    LevelZeroIOGroup::LevelZeroIOGroup()
        : LevelZeroIOGroup(platform_topo(), levelzero_device_pool())
    {
    }

    // Set up mapping between signal and control names and corresponding indices
    LevelZeroIOGroup::LevelZeroIOGroup(const PlatformTopo &platform_topo,
                                       const LevelZeroDevicePool &device_pool)
        : m_platform_topo(platform_topo)
        , m_levelzero_device_pool(device_pool)
        , m_is_batch_read(false)
        , m_signal_available({{"LEVELZERO::FREQUENCY_GPU", {
                                  "Accelerator compute/GPU domain frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      // Note: Only the domain index is changing
                                      //       here when signals are generated in
                                      //       the init function. Everything else
                                      //       is provided as part of this initial
                                      //       declaration and does not change per
                                      //       signal.
                                      return this->m_levelzero_device_pool.frequency_status(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_COMPUTE);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::FREQUENCY_MAX_GPU", {
                                  "Accelerator compute/GPU domain maximum frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.frequency_max(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_COMPUTE);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::FREQUENCY_MIN_GPU", {
                                  "Accelerator compute/GPU domain minimum frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.frequency_min(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_COMPUTE);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::ENERGY", {
                                  "Accelerator energy in Joules",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.energy(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ENERGY_TIMESTAMP", {
                                  "Accelerator energy timestamp in seconds",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.energy_timestamp(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::FREQUENCY_MEMORY", {
                                  "Accelerator memory domain frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.frequency_status(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_MEMORY);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::FREQUENCY_MAX_MEMORY", {
                                  "Accelerator memory domain maximum frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.frequency_max(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_MEMORY);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::FREQUENCY_MIN_MEMORY", {
                                  "Accelerator memory domain minimum frequency in hertz",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.frequency_min(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_MEMORY);
                                  },
                                  1e6
                                  }},
                              {"LEVELZERO::POWER_LIMIT_DEFAULT", {
                                  "Default power limit in Watts",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.power_limit_tdp(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e3
                                  }},
                              {"LEVELZERO::POWER_LIMIT_MIN", {
                                  "Minimum power limit in Watts",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.power_limit_min(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e3
                                  }},
                              {"LEVELZERO::POWER_LIMIT_MAX", {
                                  "Maximum power limit in Watts",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.power_limit_max(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e3
                                  }},
                              {"LEVELZERO::ACTIVE_TIME", {
                                  "GPU active time",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ACTIVE_TIME_TIMESTAMP", {
                                  "GPU active time reading timestamp",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time_timestamp(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_ALL);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ACTIVE_TIME_COMPUTE", {
                                  "GPU Compute engine active time",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_COMPUTE);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ACTIVE_TIME_TIMESTAMP_COMPUTE", {
                                  "GPU Compute engine active time reading timestamp",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time_timestamp(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_COMPUTE);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ACTIVE_TIME_COPY", {
                                  "GPU Copy engine active time",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_MEMORY);
                                  },
                                  1 / 1e6
                                  }},
                              {"LEVELZERO::ACTIVE_TIME_TIMESTAMP_COPY", {
                                  "GPU Copy engine active time timestamp",
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                  Agg::average,
                                  string_format_double,
                                  {},
                                  [this](unsigned int domain_idx) -> double
                                  {
                                      return this->m_levelzero_device_pool.active_time_timestamp(
                                                   GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                                   domain_idx,
                                                   geopm::LevelZero::M_DOMAIN_MEMORY);
                                  },
                                  1 / 1e6
                                  }}
                             })
        , m_control_available({{"LEVELZERO::FREQUENCY_GPU_CONTROL", {
                                    "Sets accelerator frequency (in hertz)",
                                    {},
                                    GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP,
                                    Agg::average,
                                    string_format_double
                                    }}
                              })
    {
        // populate signals for each domain
        for (auto &sv : m_signal_available) {
            std::vector<std::shared_ptr<Signal> > result;
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(
                                     signal_domain_type(sv.first)); ++domain_idx) {
                std::shared_ptr<Signal> signal =
                        std::make_shared<LevelZeroSignal>(sv.second.m_devpool_func,
                                                          domain_idx,
                                                          sv.second.scalar);
                result.push_back(signal);
            }
            sv.second.m_signals = result;
        }

        register_derivative_signals();

        register_signal_alias("FREQUENCY_ACCELERATOR", "LEVELZERO::FREQUENCY_GPU");
        register_signal_alias("POWER_ACCELERATOR", "LEVELZERO::POWER");
        register_control_alias("FREQUENCY_ACCELERATOR_CONTROL",
                               "LEVELZERO::FREQUENCY_GPU_CONTROL");

        // populate controls for each domain
        for (auto &sv : m_control_available) {
            std::vector<std::shared_ptr<control_s> > result;
            for (int domain_idx = 0;
                 domain_idx < m_platform_topo.num_domain(control_domain_type(sv.first));
                 ++domain_idx) {
                std::shared_ptr<control_s> ctrl = std::make_shared<control_s>(control_s{0, false});
                result.push_back(ctrl);
            }
            sv.second.controls = result;
        }
    }

    void LevelZeroIOGroup::register_derivative_signals(void) {
        int derivative_window = 8;
        double sleep_time = 0.005;

        struct signal_data
        {
            std::string name;
            std::string description;
            std::string base_name;
            std::string time_name;
        };

        std::vector<signal_data> derivative_signals {
            {"LEVELZERO::POWER",
                    "Average accelerator power over 40 ms or 8 control loop iterations",
                    "LEVELZERO::ENERGY",
                    "LEVELZERO::ENERGY_TIMESTAMP"},
            {"LEVELZERO::UTILIZATION",
                    "GPU utilization"
                        "n  Level Zero logical engines may may to the same hardware"
                        "\n  resulting in a reduced signal range (i.e. not 0 to 1)",
                    "LEVELZERO::ACTIVE_TIME",
                    "LEVELZERO::ACTIVE_TIME_TIMESTAMP"},
            {"LEVELZERO::UTILIZATION_COMPUTE",
                    "Compute engine utilization"
                        "n  Level Zero logical engines may may to the same hardware"
                        "\n  resulting in a reduced signal range (i.e. not 0 to 1)",
                    "LEVELZERO::ACTIVE_TIME_COMPUTE",
                    "LEVELZERO::ACTIVE_TIME_TIMESTAMP_COMPUTE"},
            {"LEVELZERO::UTILIZATION_COPY",
                    "Copy engine utilization"
                        "n  Level Zero logical engines may may to the same hardware"
                        "\n  resulting in a reduced signal range (i.e. not 0 to 1)",
                    "LEVELZERO::ACTIVE_TIME_COPY",
                    "LEVELZERO::ACTIVE_TIME_TIMESTAMP_COPY"},
        };
        for (const auto &ds : derivative_signals) {
            auto read_it = m_signal_available.find(ds.base_name);
            auto time_it = m_signal_available.find(ds.time_name);
            if (read_it != m_signal_available.end()
                && time_it != m_signal_available.end()) {
                auto readings = read_it->second.m_signals;
                int domain = read_it->second.domain_type;
                int num_domain = m_platform_topo.num_domain(domain);
                GEOPM_DEBUG_ASSERT(num_domain == (int)readings.size(),
                                   "size of domain for " + ds.base_name +
                                   " does not match number of signals available.");

                auto time_sig = time_it->second.m_signals;
                GEOPM_DEBUG_ASSERT(time_it->second.domain_type == domain,
                                   "domain for " + ds.time_name +
                                   " does not match " + ds.base_name);

                std::vector<std::shared_ptr<Signal> > result(num_domain);
                for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                    auto read = readings[domain_idx];
                    auto time = time_sig[domain_idx];
                    result[domain_idx] =
                        std::make_shared<DerivativeSignal>(time, read,
                                                           derivative_window,
                                                           sleep_time);
                }
                m_signal_available[ds.name] = {ds.description + "\n    alias_for: " +
                                               ds.base_name + " rate of change",
                                                   domain,
                                                   agg_function(ds.base_name),
                                                   format_function(ds.base_name),
                                                   result,
                                                   nullptr,
                                                   1
                                                   };
            }
        }

    }

    // Extract the set of all signal names from the index map
    std::set<std::string> LevelZeroIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Extract the set of all control names from the index map
    std::set<std::string> LevelZeroIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_control_available) {
            result.insert(sv.first);
        }
        return result;
    }

    // Check signal name using index map
    bool LevelZeroIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    // Check control name using index map
    bool LevelZeroIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_control_available.find(control_name) != m_control_available.end();
    }

    // Return domain for all valid signals
    int LevelZeroIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.domain_type;
        }
        return result;
    }

    // Return domain for all valid controls
    int LevelZeroIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_available.find(control_name);
        if (it != m_control_available.end()) {
            result = it->second.domain_type;
        }
        return result;
    }

    // Mark the given signal to be read by read_batch()
    int LevelZeroIOGroup::push_signal(const std::string &signal_name,
                                      int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": signal_name " + signal_name +
                            " not valid for LevelZeroIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": " + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 ||
            domain_idx >= m_platform_topo.num_domain(
                                          signal_domain_type(signal_name))) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_batch_read) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<Signal> signal = m_signal_available.at(
                                         signal_name).m_signals.at(domain_idx);

        // Check if signal was already pushed.
        for (size_t ii = 0; !is_found && ii < m_signal_pushed.size(); ++ii) {
            if (m_signal_pushed[ii] == signal) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed signals and configure for batch reads
            result = m_signal_pushed.size();
            m_signal_pushed.push_back(signal);
            signal->setup_batch();
        }

        return result;
    }

    // Mark the given control to be written by write_batch()
    int LevelZeroIOGroup::push_control(const std::string &control_name,
                                       int domain_type, int domain_idx)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": control_name " + control_name +
                            " not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": " + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 ||
            domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": domain_idx out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        std::shared_ptr<control_s> control = m_control_available.at(
                                             control_name).controls.at(domain_idx);

        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_control_pushed.size(); ++ii) {
            // same location means this control or its alias was already pushed
            if (m_control_pushed[ii] == control) {
                result = ii;
                is_found = true;
            }
        }
        if (!is_found) {
            // If not pushed, add to pushed control
            result = m_control_pushed.size();
            m_control_pushed.push_back(control);
        }

        return result;
    }

    // Parse and update saved values for signals
    void LevelZeroIOGroup::read_batch(void)
    {
        m_is_batch_read = true;
        // TODO: if ENERGY and ENERGY_TIMESTAMP or ACTIVE_TIME and
        //       ACTIVE_TIMESTAMP signals are called at the same domain
        //       level, instead call the paired version to guarantee we get the
        //       value with the correct timestamp
    }

    // Write all controls that have been pushed and adjusted
    void LevelZeroIOGroup::write_batch(void)
    {
        for (auto &sv : m_control_available) {
            for (unsigned int domain_idx = 0;
                 domain_idx < sv.second.controls.size(); ++domain_idx) {
                if (sv.second.controls.at(domain_idx)->m_is_adjusted) {
                    write_control(sv.first, sv.second.domain_type, domain_idx,
                                  sv.second.controls.at(domain_idx)->m_setting);
                }
            }
        }
    }

    // Return the latest value read by read_batch()
    double LevelZeroIOGroup::sample(int batch_idx)
    {
        if (batch_idx < 0 || batch_idx >= (int)m_signal_pushed.size()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": batch_idx " + std::to_string(batch_idx) +
                            " out of range", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);

        }
        //Not strictly necessary, but keeping to enforce general
        //flow of read_batch followed by sample.
        if (!m_is_batch_read) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": signal has not been read.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_signal_pushed[batch_idx]->sample();
    }

    // Save a setting to be written by a future write_batch()
    void LevelZeroIOGroup::adjust(int batch_idx, double setting)
    {
        if (batch_idx < 0 || (unsigned)batch_idx >= m_control_pushed.size()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            "(): batch_idx " + std::to_string(batch_idx) +
                            " out of range", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }

        m_control_pushed.at(batch_idx)->m_setting = setting;
        m_control_pushed.at(batch_idx)->m_is_adjusted = true;
    }

    // Read the value of a signal immediately, bypassing read_batch().
    // Should not modify m_signal_value
    double LevelZeroIOGroup::read_signal(const std::string &signal_name,
                                         int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": " + signal_name + " not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": "
                            + signal_name + ": domain_type must be " +
                            std::to_string(signal_domain_type(signal_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 ||
            domain_idx >= m_platform_topo.num_domain(signal_domain_type(signal_name))) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": domain_idx out of range.", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }

        double result = NAN;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = (it->second.m_signals.at(domain_idx))->read();
        }
        else {
    #ifdef GEOPM_DEBUG
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": Handling not defined for " + signal_name,
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    #endif
        }
        return result;
    }

    // Write to the control immediately, bypassing write_batch()
    void LevelZeroIOGroup::write_control(const std::string &control_name,
                                         int domain_type, int domain_idx,
                                         double setting)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": "
                            + control_name + " not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": "
                            + control_name + ": domain_type must be " +
                            std::to_string(control_domain_type(control_name)),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 ||
            domain_idx >= m_platform_topo.num_domain(
                                          control_domain_type(control_name))) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": domain_idx out of range.", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }

        if (control_name == "LEVELZERO::FREQUENCY_GPU_CONTROL" ||
            control_name == "FREQUENCY_ACCELERATOR_CONTROL") {
            m_levelzero_device_pool.frequency_control(domain_type, domain_idx,
                                                      geopm::LevelZero::M_DOMAIN_COMPUTE,
                                                      setting / 1e6);
        }
        else {
    #ifdef GEOPM_DEBUG
                throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                                "Handling not defined for " + control_name,
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    #endif
        }
    }

    // Implemented to allow an IOGroup to save platform settings before starting
    // to adjust them
    void LevelZeroIOGroup::save_control(void)
    {
        for (int domain_idx = 0;
             domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR);
             ++domain_idx) {
            // TODO: Read ALL LEVELZERO Power Limits, frequency settings, etc
        }
    }

    // Implemented to allow an IOGroup to restore previously saved
    // platform settings
    void LevelZeroIOGroup::restore_control(void)
    {
        /// @todo: Usage of the LEVELZERO API for setting frequency, power, etc
        ///        requires root privileges.  As such several unit tests will
        ///        fail when calling restore_control.  Once a non-privileged
        ///        solution is available this may be used
    }

    // Hint to Agent about how to aggregate signals from this IOGroup
    std::function<double(const std::vector<double> &)> LevelZeroIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": "
                            + signal_name + "not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_agg_function;
    }

    // Specifies how to print signals from this IOGroup
    std::function<std::string(double)> LevelZeroIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": "
                            + signal_name + "not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_format_function;
    }

    // A user-friendly description of each signal
    std::string LevelZeroIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": signal_name " + signal_name +
                            " not valid for LevelZeroIOGroup.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.at(signal_name).m_description;
    }

    // A user-friendly description of each control
    std::string LevelZeroIOGroup::control_description(const std::string &control_name) const
    {
        if (!is_valid_control(control_name)) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) + ": " +
                            control_name + "not valid for LevelZeroIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_control_available.at(control_name).m_description;
    }

    // Name used for registration with the IOGroup factory
    std::string LevelZeroIOGroup::plugin_name(void)
    {
        return "levelzero";
    }

    int LevelZeroIOGroup::signal_behavior(const std::string &signal_name) const
    {
        return IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE;
    }

    // Function used by the factory to create objects of this type
    std::unique_ptr<IOGroup> LevelZeroIOGroup::make_plugin(void)
    {
        return geopm::make_unique<LevelZeroIOGroup>();
    }

    void LevelZeroIOGroup::register_signal_alias(const std::string &alias_name,
                                            const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": signal_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            // skip adding an alias if underlying signal is not found
            return;
        }
        // copy signal info but append to description
        m_signal_available[alias_name] = it->second;
        m_signal_available[alias_name].m_description =
            m_signal_available[signal_name].m_description + '\n' +
                                            "    alias_for: " + signal_name;
    }

    void LevelZeroIOGroup::register_control_alias(const std::string &alias_name,
                                           const std::string &control_name)
    {
        if (m_control_available.find(alias_name) != m_control_available.end()) {
            throw Exception("LevelZeroIOGroup::" + std::string(__func__) +
                            ": contro1_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_control_available.find(control_name);
        if (it == m_control_available.end()) {
            // skip adding an alias if underlying control is not found
            return;
        }
        // copy control info but append to description
        m_control_available[alias_name] = it->second;
        m_control_available[alias_name].m_description =
       m_control_available[control_name].m_description + '\n' +
                                         "    alias_for: " + control_name;
    }
}
