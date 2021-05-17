/*
 * Copyright (c) 2020, Intel Corporation
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
#include "PAPIIOGroup.hpp"

#include <papi.h>
#include <unistd.h>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>

#include "Agg.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "IOGroup.hpp"
#include "PlatformTopo.hpp"

namespace geopm
{
    [[noreturn]] static void throw_exception_from_papi(const std::string &throwing_function,
                                                       const std::string &file,
                                                       int line,
                                                       const std::string &papi_call,
                                                       int retval)
    {
        std::ostringstream oss;
        oss << throwing_function << ": ";
        int error = GEOPM_ERROR_RUNTIME;
        if (retval == PAPI_ESYS) {
            error = errno;
            oss << "System error in " << papi_call;
        }
        else {
            oss << "Error in " << papi_call << "(" << retval
                << "): " << PAPI_strerror(retval);
        }
        throw Exception(oss.str(), error, file.c_str(), line);
    }

    PAPIIOGroup::PAPIIOGroup()
        : m_signals()
        , m_papi_values_per_core()
        , m_batch_values()
        , m_papi_event_sets()
    {

        std::vector<std::string> event_names;
        const char *env_str = std::getenv("GEOPM_PAPI_EVENTS");
        if (env_str) {
            std::stringstream ss(env_str);
            std::istream_iterator<std::string> begin(ss);
            std::istream_iterator<std::string> end;
            // Get a whitespace-delimited list of events to initialize in PAPI
            event_names = std::vector<std::string>(begin, end);
        }

        int retval;

        // Initialize PAPI and our multiplexed events set
        retval = PAPI_library_init(PAPI_VER_CURRENT);
        if (retval != PAPI_VER_CURRENT) {
            throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__,
                                      __LINE__, "PAPI_library_init", retval);
        }

        retval = PAPI_multiplex_init();
        if (retval != PAPI_OK) {
            throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__,
                                      __LINE__, "PAPI_multiplex_init", retval);
        }

        retval = PAPI_set_granularity(PAPI_GRN_SYS);
        if (retval != PAPI_OK) {
            throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__, __LINE__,
                                      "PAPI_set_granularity(PAPI_GRN_SYS)", retval);
        }

        const auto *papi_hardware = PAPI_get_hardware_info();
        int num_cores = papi_hardware->sockets * papi_hardware->cores;
        m_papi_event_sets = std::vector<int>(num_cores, PAPI_NULL);

        // TODO: Can we get pids of our other processes and use PAPI_attach? Would
        // that avoid the permissions we need for system-wide monitoring?
        for (int i = 0; i < num_cores; ++i) {
            retval = PAPI_create_eventset(&m_papi_event_sets[i]);
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__,
                                          __LINE__, "PAPI_create_eventset", retval);
            }

            retval = PAPI_assign_eventset_component(m_papi_event_sets[i], 0);
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__, __LINE__,
                                          "PAPI_assign_eventset_component", retval);
            }

            PAPI_option_t opt;
            opt.cpu.cpu_num = i;
            opt.cpu.eventset = m_papi_event_sets[i];
            retval = PAPI_set_opt(PAPI_CPU_ATTACH, &opt);
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__, __LINE__,
                                          "PAPI_set_opt(PAPI_CPU_ATTACH)", retval);
            }

            retval = PAPI_set_multiplex(m_papi_event_sets[i]);
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__,
                                          __LINE__, "PAPI_set_multiplex", retval);
            }

            // Add events to the PAPI event set
            for (const auto &event_name : event_names) {
                int event_code = PAPI_NULL;
                retval = PAPI_event_name_to_code(event_name.c_str(), &event_code);
                if (retval != PAPI_OK) {
                    std::ostringstream oss;
                    oss << "PAPI_event_name_to_code(\"" << event_name << "\")";
                    throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()",
                                              __FILE__, __LINE__, oss.str(), retval);
                }

                retval = PAPI_add_event(m_papi_event_sets[i], event_code);
                if (retval != PAPI_OK) {
                    std::ostringstream oss;
                    oss << "PAPI_add_event(\"" << event_name << "\")";
                    throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()",
                                              __FILE__, __LINE__, oss.str(), retval);
                }
            }

            m_papi_values_per_core.emplace_back(
                std::vector<long long>(event_names.size(), 0));

            retval = PAPI_start(m_papi_event_sets[i]);
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()", __FILE__, __LINE__,
                                          "PAPI_start CPU " + std::to_string(i), retval);
            }
        }

        m_batch_values = std::vector<double>(num_cores * event_names.size(), 0);

        // Add events to the PAPI event set
        for (size_t i = 0; i < event_names.size(); ++i) {
            const auto &event_name = event_names[i];

            int event_code = PAPI_NULL;
            retval = PAPI_event_name_to_code(event_name.c_str(), &event_code);
            if (retval != PAPI_OK) {
                std::ostringstream oss;
                oss << "PAPI_event_name_to_code(\"" << event_name << "\")";
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()",
                        __FILE__, __LINE__, oss.str(), retval);
            }

            PAPI_event_info_t event_info = {};
            PAPI_get_event_info(event_code, &event_info);
            if (retval != PAPI_OK) {
                std::ostringstream oss;
                oss << "get_event_info(\"" << event_name << "\")";
                throw_exception_from_papi("PAPIIOGroup::PAPIIOGroup()",
                        __FILE__, __LINE__, oss.str(), retval);
            }

            m_signals.emplace(event_name, signal_s{i, std::string(event_info.long_descr)});
        }
    }

    std::set<std::string> PAPIIOGroup::signal_names(void) const
    {
        std::set<std::string> names;
        for (const auto &signal_offset_kv : m_signals) {
            names.insert(signal_offset_kv.first);
        }
        return names;
    }

    std::set<std::string> PAPIIOGroup::control_names(void) const
    {
        return {};
    }

    bool PAPIIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signals.find(signal_name) != m_signals.end();
    }

    bool PAPIIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int PAPIIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        return is_valid_signal(signal_name) ? GEOPM_DOMAIN_CORE : GEOPM_DOMAIN_INVALID;
    }

    int PAPIIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int PAPIIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        auto signal_it = m_signals.find(signal_name);
        if (signal_it == m_signals.end()) {
            throw Exception("PAPIIOGroup::push_signal(): " + signal_name +
                                "not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_CORE) {
            throw Exception("PAPIIOGroup::push_signal(): domain_type " +
                                std::to_string(domain_type) +
                                "not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return domain_idx * m_signals.size() + signal_it->second.m_papi_offset;
    }

    int PAPIIOGroup::push_control(const std::string &control_name, int domain_type,
                                  int domain_idx)
    {
        throw Exception("PAPIIOGroup has no controls", GEOPM_ERROR_INVALID,
                        __FILE__, __LINE__);
    }

    void PAPIIOGroup::read_batch(void)
    {
        for (size_t core = 0; core < m_papi_event_sets.size(); ++core) {
            int retval = PAPI_read(m_papi_event_sets[core],
                                   m_papi_values_per_core[core].data());
            if (retval != PAPI_OK) {
                throw_exception_from_papi("PAPIIOGroup::read_batch()", __FILE__,
                                          __LINE__, "PAPI_read", retval);
            }

            for (size_t i = 0; i < m_signals.size(); ++i) {
                m_batch_values[core * m_signals.size() + i] =
                    static_cast<double>(m_papi_values_per_core[core][i]);
            }
        }
    }

    void PAPIIOGroup::write_batch(void) {}

    double PAPIIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        if (batch_idx < 0 || batch_idx >= static_cast<int>(m_batch_values.size())) {
            throw Exception("PAPIIOGroup::sample(): batch_idx " + std::to_string(batch_idx) +
                                " not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else {
            result = m_batch_values[batch_idx];
        }
        return result;
    }

    void PAPIIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("PAPIIOGroup has no controls", GEOPM_ERROR_INVALID,
                        __FILE__, __LINE__);
    }

    double PAPIIOGroup::read_signal(const std::string &signal_name, int domain_type,
                                    int domain_idx)
    {
        auto signal_it = m_signals.find(signal_name);
        if (signal_it == m_signals.end()) {
            throw Exception("PAPIIOGroup::read_signal(): " + signal_name +
                                "not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_CORE) {
            throw Exception("PAPIIOGroup:read_signal(): domain_type " +
                                std::to_string(domain_type) +
                                "not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int retval = PAPI_read(m_papi_event_sets[domain_idx],
                               m_papi_values_per_core[domain_idx].data());
        if (retval != PAPI_OK) {
            throw_exception_from_papi("PAPIIOGroup::read_signal()", __FILE__,
                                      __LINE__, "PAPI_read", retval);
        }

        return static_cast<double>(m_papi_values_per_core[domain_idx][signal_it->second.m_papi_offset]);
    }

    void PAPIIOGroup::write_control(const std::string &control_name,
                                    int domain_type, int domain_idx, double setting)
    {
        throw Exception("PAPIIOGroup has no controls", GEOPM_ERROR_INVALID,
                        __FILE__, __LINE__);
    }

    void PAPIIOGroup::save_control(void) {}

    void PAPIIOGroup::restore_control(void) {}

    std::function<double(const std::vector<double> &)>
        PAPIIOGroup::agg_function(const std::string &signal_name) const
    {
        // All signals will be aggregated as a sum for now
        return geopm::Agg::sum;
    }

    std::function<std::string(double)>
        PAPIIOGroup::format_function(const std::string &signal_name) const
    {
        return geopm::string_format_integer;
    }

    std::string PAPIIOGroup::signal_description(const std::string &signal_name) const
    {
        auto signal_it = m_signals.find(signal_name);
        if (signal_it == m_signals.end()) {
            throw Exception("PAPIIOGroup::signal_description(): " + signal_name +
                                "not valid for PAPIIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Alternatively, see papi_avail and papi_native_avail
        return signal_it->second.m_description;
    }

    std::string PAPIIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("PAPIIOGroup has no controls", GEOPM_ERROR_INVALID,
                        __FILE__, __LINE__);
    }

    int PAPIIOGroup::signal_behavior(const std::string &signal_name) const
    {
        // TODO: PAPI_event_info_t::value_type could decide our agg function.
        //       (PAPI_VALUETYPE_RUNNING_SUM or PAPI_VALUETYPE_ABSOLUTE)
        //       most of them are running sum type
        return IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE;
    }

    std::string PAPIIOGroup::plugin_name(void)
    {
        return "PAPI";
    }

    std::unique_ptr<geopm::IOGroup> PAPIIOGroup::make_plugin(void)
    {
        return std::unique_ptr<geopm::IOGroup>(new PAPIIOGroup);
    }
}
