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

#include "ProfileIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "EpochRuntimeRegulator.hpp"
#include "RuntimeRegulator.hpp"
#include "ProfileIOSample.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "geopm_hash.h"
#include "geopm_time.h"
#include "config.h"

#define GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME "PROFILE"

namespace geopm
{
    ProfileIOGroup::ProfileIOGroup(std::shared_ptr<IProfileIOSample> profile_sample,
                                     IEpochRuntimeRegulator &epoch_regulator)
        : ProfileIOGroup(profile_sample, epoch_regulator, platform_topo())
    {

    }

    ProfileIOGroup::ProfileIOGroup(std::shared_ptr<IProfileIOSample> profile_sample,
                                     IEpochRuntimeRegulator &epoch_regulator,
                                     IPlatformTopo &topo)
        : m_profile_sample(profile_sample)
        , m_epoch_regulator(epoch_regulator)
        , m_signal_idx_map{{plugin_name() + "::REGION_ID#", M_SIGNAL_REGION_ID},
                           {plugin_name() + "::REGION_HASH", M_SIGNAL_REGION_HASH},
                           {plugin_name() + "::REGION_HINT", M_SIGNAL_REGION_HINT},
                           {plugin_name() + "::REGION_PROGRESS", M_SIGNAL_REGION_PROGRESS},
                           {plugin_name() + "::REGION_THREAD_PROGRESS", M_SIGNAL_THREAD_PROGRESS},
                           {"REGION_ID#", M_SIGNAL_REGION_ID},
                           {"REGION_HASH", M_SIGNAL_REGION_HASH},
                           {"REGION_HINT", M_SIGNAL_REGION_HINT},
                           {"REGION_PROGRESS", M_SIGNAL_REGION_PROGRESS},
                           {"REGION_THREAD_PROGRESS", M_SIGNAL_THREAD_PROGRESS},
                           {plugin_name() + "::EPOCH_RUNTIME", M_SIGNAL_EPOCH_RUNTIME},
                           {"EPOCH_RUNTIME", M_SIGNAL_EPOCH_RUNTIME},
                           {plugin_name() + "::EPOCH_COUNT", M_SIGNAL_EPOCH_COUNT},
                           {"EPOCH_COUNT", M_SIGNAL_EPOCH_COUNT},
                           {plugin_name() + "::REGION_RUNTIME", M_SIGNAL_RUNTIME},
                           {"REGION_RUNTIME", M_SIGNAL_RUNTIME},
                           {plugin_name() + "::EPOCH_RUNTIME_MPI", M_SIGNAL_EPOCH_RUNTIME_MPI},
                           {"EPOCH_RUNTIME_MPI", M_SIGNAL_EPOCH_RUNTIME_MPI},
                           {plugin_name() + "::EPOCH_RUNTIME_IGNORE", M_SIGNAL_EPOCH_RUNTIME_IGNORE},
                           {"EPOCH_RUNTIME_IGNORE", M_SIGNAL_EPOCH_RUNTIME_IGNORE}}
        , m_platform_topo(topo)
        , m_do_read(M_SIGNAL_MAX, false)
        , m_per_cpu_progress(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), NAN)
        , m_per_cpu_runtime(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), NAN)
        , m_thread_progress(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), NAN)
        , m_epoch_runtime_mpi(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), 0.0)
        , m_epoch_runtime_ignore(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), 0.0)
        , m_epoch_runtime(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), 0.0)
        , m_epoch_count(topo.num_domain(IPlatformTopo::M_DOMAIN_CPU), 0.0)
        , m_cpu_rank(m_profile_sample->cpu_rank())
    {

    }

    ProfileIOGroup::~ProfileIOGroup()
    {

    }

    std::set<std::string> ProfileIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_idx_map) {
            result.insert(sv.first);
        }
        return result;
    }

    std::set<std::string> ProfileIOGroup::control_names(void) const
    {
        return {};
    }

    bool ProfileIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_idx_map.find(signal_name) != m_signal_idx_map.end();
    }

    bool ProfileIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int ProfileIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = IPlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = IPlatformTopo::M_DOMAIN_CPU;
        }
        return result;
    }

    int ProfileIOGroup::control_domain_type(const std::string &control_name) const
    {
        return PlatformTopo::M_DOMAIN_INVALID;
    }

    int ProfileIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        int result = -1;
        if (m_is_batch_read) {
            throw Exception("ProfileIOGroup::push_signal: cannot push signal after call to read_batch().",
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
            m_do_read[signal_type] = true;
            // Runtime signal requires region_id as well
            if (signal_type == M_SIGNAL_RUNTIME) {
                m_do_read[M_SIGNAL_REGION_ID] = true;
            }
        }
        return result;
    }

    int ProfileIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("ProfileIOGroup::push_control() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ProfileIOGroup::read_batch(void)
    {
        if (m_do_read[M_SIGNAL_REGION_ID] ||
            m_do_read[M_SIGNAL_REGION_HASH] ||
            m_do_read[M_SIGNAL_REGION_HINT]) {
            m_per_cpu_region_id = m_profile_sample->per_cpu_region_id();
        }
        if (m_do_read[M_SIGNAL_REGION_PROGRESS]) {
            struct geopm_time_s read_time;
            geopm_time(&read_time);
            m_per_cpu_progress = m_profile_sample->per_cpu_progress(read_time);
        }
        if (m_do_read[M_SIGNAL_THREAD_PROGRESS]) {
            m_thread_progress = m_profile_sample->per_cpu_thread_progress();
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME]) {
            std::vector<double> per_rank_epoch_runtime = m_epoch_regulator.last_epoch_runtime();
            for (size_t cpu_idx = 0; cpu_idx != m_cpu_rank.size(); ++cpu_idx) {
                m_epoch_runtime[cpu_idx] = per_rank_epoch_runtime[m_cpu_rank[cpu_idx]];
            }
        }
        if (m_do_read[M_SIGNAL_EPOCH_COUNT]) {
            std::vector<double> per_rank_epoch_count = m_epoch_regulator.epoch_count();
            for (size_t cpu_idx = 0; cpu_idx != m_cpu_rank.size(); ++cpu_idx) {
                m_epoch_count[cpu_idx] = per_rank_epoch_count[m_cpu_rank[cpu_idx]];
            }
        }
        if (m_do_read[M_SIGNAL_RUNTIME]) {
            // look up the region for each cpu and cache the per-cpu runtimes for that region
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
            // look up the last runtime for a cpu given its current region
            // we assume ranks don't move between cpus
            for (size_t cpu = 0; cpu < m_per_cpu_runtime.size(); ++cpu) {
                m_per_cpu_runtime[cpu] = cache.at(m_per_cpu_region_id[cpu])[cpu];
            }
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME_MPI]) {
            std::vector<double> per_rank_epoch_runtime_mpi = m_epoch_regulator.last_epoch_runtime_mpi();
            for (size_t cpu_idx = 0; cpu_idx != m_cpu_rank.size(); ++cpu_idx) {
                m_epoch_runtime_mpi[cpu_idx] = per_rank_epoch_runtime_mpi[m_cpu_rank[cpu_idx]];
            }
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME_IGNORE]) {
            std::vector<double> per_rank_epoch_runtime_ignore = m_epoch_regulator.last_epoch_runtime_ignore();
            for (size_t cpu_idx = 0; cpu_idx != m_cpu_rank.size(); ++cpu_idx) {
                m_epoch_runtime_ignore[cpu_idx] = per_rank_epoch_runtime_ignore[m_cpu_rank[cpu_idx]];
            }
        }
        m_is_batch_read = true;
    }

    void ProfileIOGroup::write_batch(void)
    {

    }

    double ProfileIOGroup::sample(int signal_idx)
    {
        double result = NAN;
        if (signal_idx < 0 || signal_idx >= (int)m_active_signal.size()) {
            throw Exception("ProfileIOGroup::sample(): signal_idx out of range",
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
            case M_SIGNAL_REGION_HASH:
                result = geopm_region_id_hash(m_per_cpu_region_id[cpu_idx]);
                break;
            case M_SIGNAL_REGION_HINT:
                result = geopm_region_id_hint(m_per_cpu_region_id[cpu_idx]);
                break;
            case M_SIGNAL_REGION_PROGRESS:
                result = m_per_cpu_progress[cpu_idx];
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_thread_progress[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME:
                result = m_epoch_runtime[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_COUNT:
                result = m_epoch_count[cpu_idx];
                break;
            case M_SIGNAL_RUNTIME:
                result = m_per_cpu_runtime[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_MPI:
                result = m_epoch_runtime_mpi[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_IGNORE:
                result = m_epoch_runtime_ignore[cpu_idx];
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("ProfileIOGroup:sample(): Signal was pushed with an invalid signal type",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }

        return result;
    }

    void ProfileIOGroup::adjust(int control_idx, double setting)
    {
        throw Exception("ProfileIOGroup::adjust() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double ProfileIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
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
            case M_SIGNAL_REGION_HASH:
                result = geopm_region_id_hash(m_profile_sample->per_cpu_region_id()[cpu_idx]);
                break;
            case M_SIGNAL_REGION_HINT:
                result = geopm_region_id_hint(m_profile_sample->per_cpu_region_id()[cpu_idx]);
                break;
            case M_SIGNAL_REGION_PROGRESS:
                geopm_time(&read_time);
                result = m_profile_sample->per_cpu_progress(read_time)[cpu_idx];
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_profile_sample->per_cpu_thread_progress()[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME:
                result = m_epoch_regulator.last_epoch_runtime()[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_COUNT:
                result = m_epoch_regulator.epoch_count()[cpu_idx];
                break;
            case M_SIGNAL_RUNTIME:
                region_id = m_profile_sample->per_cpu_region_id()[cpu_idx];
                result = m_profile_sample->per_cpu_runtime(region_id)[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_MPI:
                result = m_epoch_regulator.last_epoch_runtime_mpi()[cpu_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_IGNORE:
                result = m_epoch_regulator.last_epoch_runtime_ignore()[cpu_idx];
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("ProfileIOGroup:read_signal(): Invalid signal type bug check_signal did not throw",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
                break;
        }
        return result;
    }

    void ProfileIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("ProfileIOGroup::write_control() there are no controls supported by the ProfileIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ProfileIOGroup::save_control(void)
    {

    }

    void ProfileIOGroup::restore_control(void)
    {

    }

    std::function<double(const std::vector<double> &)> ProfileIOGroup::agg_function(const std::string &signal_name) const
    {
        static const std::map<std::string, std::function<double(const std::vector<double> &)> > fn_map {
            {"REGION_RUNTIME", Agg::max},
            {"PROFILE::REGION_RUNTIME", Agg::max},
            {"REGION_PROGRESS", Agg::min},
            {"PROFILE::REGION_PROGRESS", Agg::min},
            {"REGION_THREAD_PROGRESS", Agg::min},
            {"PROFILE::REGION_THREAD_PROGRESS", Agg::min},
            {"REGION_ID#", Agg::region_id},
            {"PROFILE::REGION_ID#", Agg::region_id},
            {"REGION_HASH", Agg::region_hash},
            {"PROFILE::REGION_HASH", Agg::region_hash},
            {"REGION_HINT", Agg::region_hint},
            {"PROFILE::REGION_HINT", Agg::region_hint},
            {"EPOCH_RUNTIME", Agg::max},
            {"PROFILE::EPOCH_RUNTIME", Agg::max},
            {"EPOCH_ENERGY", Agg::sum},
            {"PROFILE::EPOCH_ENERGY", Agg::sum},
            {"EPOCH_COUNT", Agg::min},
            {"PROFILE::EPOCH_COUNT", Agg::min},
            {"EPOCH_RUNTIME_MPI", Agg::max},
            {"PROFILE::EPOCH_RUNTIME_MPI", Agg::max},
            {"EPOCH_RUNTIME_IGNORE", Agg::max},
            {"PROFILE::EPOCH_RUNTIME_IGNORE", Agg::max}
        };
        auto it = fn_map.find(signal_name);
        if (it == fn_map.end()) {
            throw Exception("ProfileIOGroup::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::string ProfileIOGroup::signal_description(const std::string &signal_name) const
    {
        return "";
    }

    std::string ProfileIOGroup::control_description(const std::string &control_name) const
    {
        return "";
    }

    int ProfileIOGroup::check_signal(const std::string &signal_name, int domain_type, int domain_idx) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ProfileIOGroup::check_signal(): signal_name " + signal_name +
                            " not valid for ProfileIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != PlatformTopo::M_DOMAIN_CPU) {
            /// @todo Add support for non-cpu domains.
            throw Exception("ProfileIOGroup::check_signal(): non-CPU domains are not supported",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        int cpu_idx = domain_idx;
        if (cpu_idx < 0 || cpu_idx >= m_platform_topo.num_domain(PlatformTopo::M_DOMAIN_CPU)) {
            throw Exception("ProfileIOGroup::check_signal(): domain index out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int signal_type = -1;
        auto it = m_signal_idx_map.find(signal_name);
        if (it != m_signal_idx_map.end()) {
            signal_type = it->second;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("ProfileIOGroup::check_signal: is_valid_signal() returned true, but signal name is unknown",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return signal_type;
    }

    std::string ProfileIOGroup::plugin_name(void)
    {
        return GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME;
    }
}
