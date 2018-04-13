/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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


#include "KprofileIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "KprofileIOSample.hpp"
#include "Exception.hpp"
#include "geopm_hash.h"
#include "config.h"

#define GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME "KPROFILE"

namespace geopm
{
    KprofileIOGroup::KprofileIOGroup(std::shared_ptr<IKprofileIOSample> profile_sample)
        : KprofileIOGroup(profile_sample, platform_topo())
    {

    }

    KprofileIOGroup::KprofileIOGroup(std::shared_ptr<IKprofileIOSample> profile_sample,
                                     IPlatformTopo &topo)
        : m_profile_sample(profile_sample)
        , m_signal_idx_map{{plugin_name() + "::REGION_ID#", M_SIGNAL_REGION_ID},
                           {plugin_name() + "::REGION_PROGRESS", M_SIGNAL_PROGRESS},
                           {"REGION_ID#", M_SIGNAL_REGION_ID},
                           {"REGION_PROGRESS", M_SIGNAL_PROGRESS},
                           {plugin_name() + "::REGION_RUNTIME", M_SIGNAL_RUNTIME},
                           {"REGION_RUNTIME", M_SIGNAL_RUNTIME}}
        , m_platform_topo(topo)
        , m_per_cpu_runtime(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), NAN)
    {

    }

    KprofileIOGroup::~KprofileIOGroup()
    {

    }

    bool KprofileIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
    }

    bool KprofileIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int KprofileIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = IPlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = IPlatformTopo::M_DOMAIN_CPU;
        }
        return result;
    }

    int KprofileIOGroup::control_domain_type(const std::string &control_name) const
    {
        return PlatformTopo::M_DOMAIN_INVALID;
    }

    int KprofileIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        int result = -1;
        if (m_is_batch_read) {
            throw Exception("KprofileIOGroup::push_signal: cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int signal_type = check_signal(signal_name, domain_type, domain_idx);

        int signal_idx = 0;
        for (const auto &it : m_active_signal) {
            if (it.signal_type == signal_type &&
                it.domain_type == domain_type &&
                it.domain_idx == domain_idx) {
                result = signal_idx;
            }
            ++signal_idx;
        }
        if (result == -1) {
            result = m_active_signal.size();
            m_active_signal.push_back({signal_type, domain_type, domain_idx});
            switch (signal_type) {
                case M_SIGNAL_REGION_ID:
                    m_do_read_region_id = true;
                    break;
                case M_SIGNAL_PROGRESS:
                    m_do_read_progress = true;
                    break;
                case M_SIGNAL_RUNTIME:
                    // Runtime signal requires region_id as well
                    m_do_read_region_id = true;
                    m_do_read_runtime = true;
                    break;
                default:
                    break;
            }
        }
        return result;
    }

    int KprofileIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("KprofileIOGroup::push_control() there are no controls supported by the KprofileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void KprofileIOGroup::read_batch(void)
    {
        if (m_do_read_region_id) {
            m_per_cpu_region_id = m_profile_sample->per_cpu_region_id();
        }
        if (m_do_read_progress) {
            struct geopm_time_s read_time;
            geopm_time(&read_time);
            m_per_cpu_progress = m_profile_sample->per_cpu_progress(read_time);
        }
        if (m_do_read_runtime) {
            std::map<uint64_t, std::vector<double> > cache;
            for (auto rid : m_per_cpu_region_id) {
                // add runtimes for each region if not already present
                auto it = cache.find(rid);
                if (it == cache.end()) {
                    cache.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(rid),
                                  std::forward_as_tuple(m_profile_sample->per_cpu_runtime(rid)));
                }
            }
            for (size_t cpu = 0; cpu < m_per_cpu_runtime.size(); ++cpu) {
                m_per_cpu_runtime[cpu] = cache.at(m_per_cpu_region_id[cpu])[cpu];
            }
        }
        m_is_batch_read = true;
    }

    void KprofileIOGroup::write_batch(void)
    {

    }

    double KprofileIOGroup::sample(int signal_idx) const
    {
        double result = NAN;
        if (signal_idx < 0 || signal_idx >= (int)m_active_signal.size()) {
            throw Exception("KprofileIOGroup::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("TimeIOGroup::sample(): signal has not been read",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        /// @todo support for non-cpu signal domains
        int cpu_idx = m_active_signal[signal_idx].domain_idx;
        switch (m_active_signal[signal_idx].signal_type) {
            case M_SIGNAL_REGION_ID:
                result = geopm_field_to_signal(m_per_cpu_region_id[cpu_idx]);
                break;
            case M_SIGNAL_PROGRESS:
                result = m_per_cpu_progress[cpu_idx];
                break;
            case M_SIGNAL_RUNTIME:
                result = m_per_cpu_runtime[cpu_idx];
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("KprofileIOGroup:sample(): Signal was pushed with an invalid signal type",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }

        return result;
    }

    void KprofileIOGroup::adjust(int control_idx, double setting)
    {
        throw Exception("KprofileIOGroup::adjust() there are no controls supported by the KprofileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double KprofileIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx) const
    {
        int signal_type = check_signal(signal_name, domain_type, domain_idx);
        /// @todo Add support for non-cpu domains.
        int cpu_idx = domain_idx;
        struct geopm_time_s read_time;
        uint64_t region_id;
        double result = NAN;
        switch (signal_type) {
            case M_SIGNAL_REGION_ID:
                result = geopm_field_to_signal(m_profile_sample->per_cpu_region_id()[cpu_idx]);
                break;
            case M_SIGNAL_PROGRESS:
                geopm_time(&read_time);
                result = m_profile_sample->per_cpu_progress(read_time)[cpu_idx];
                break;
            case M_SIGNAL_RUNTIME:
                region_id = m_profile_sample->per_cpu_region_id()[cpu_idx];
                result = m_profile_sample->per_cpu_runtime(region_id)[cpu_idx];
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("KprofileIOGroup:read_signal(): Invalid signal type bug check_signal did not throw",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }
        return result;
    }

    void KprofileIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("KprofileIOGroup::write_control() there are no controls supported by the KprofileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int KprofileIOGroup::check_signal(const std::string &signal_name, int domain_type, int domain_idx) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("KprofileIOGroup::check_signal(): signal_name " + signal_name +
                            " not valid for KprofileIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != PlatformTopo::M_DOMAIN_CPU) {
            /// @todo Add support for non-cpu domains.
            throw Exception("KprofileIOGroup::check_signal(): non-CPU domains are not supported",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        int cpu_idx = domain_idx;
        if (cpu_idx < 0 || cpu_idx >= m_platform_topo.num_domain(PlatformTopo::M_DOMAIN_CPU)) {
            throw Exception("KprofileIOGroup::check_signal(): domain index out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int signal_type = -1;
        auto it = m_signal_idx_map.find(signal_name);
        if (it != m_signal_idx_map.end()) {
            signal_type = it->second;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("KprofileIOGroup::check_signal: is_valid_signal() returned true, but signal name is unknown",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return signal_type;
    }

    std::string KprofileIOGroup::plugin_name(void)
    {
        return GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME;
    }
}
