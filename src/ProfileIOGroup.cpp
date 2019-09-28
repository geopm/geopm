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

#include <algorithm>

#include "ProfileIOGroup.hpp"

#include "PlatformTopoImp.hpp"
#include "Helper.hpp"
#include "EpochRuntimeRegulator.hpp"
#include "RuntimeRegulator.hpp"
#include "ProfileIOSample.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "geopm_hash.h"
#include "geopm_time.h"
#include "geopm_internal.h"
#include "config.h"

#define GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME "PROFILE"

namespace geopm
{
    ProfileIOGroup::ProfileIOGroup(std::shared_ptr<ProfileIOSample> profile_sample,
                                   EpochRuntimeRegulator &epoch_regulator)
        : ProfileIOGroup(profile_sample, epoch_regulator, platform_topo_internal())
    {

    }

    ProfileIOGroup::ProfileIOGroup(std::shared_ptr<ProfileIOSample> profile_sample,
                                   EpochRuntimeRegulator &epoch_regulator,
                                   PlatformTopoImp &topo)
        : m_profile_sample(profile_sample)
        , m_epoch_regulator(epoch_regulator)
        , m_signal_idx_map{{plugin_name() + "::REGION_HASH", M_SIGNAL_REGION_HASH},
                           {plugin_name() + "::REGION_HINT", M_SIGNAL_REGION_HINT},
                           {plugin_name() + "::REGION_PROGRESS", M_SIGNAL_REGION_PROGRESS},
                           {plugin_name() + "::REGION_COUNT", M_SIGNAL_REGION_COUNT},
                           {plugin_name() + "::REGION_THREAD_PROGRESS", M_SIGNAL_THREAD_PROGRESS},
                           {"REGION_HASH", M_SIGNAL_REGION_HASH},
                           {"REGION_HINT", M_SIGNAL_REGION_HINT},
                           {"REGION_PROGRESS", M_SIGNAL_REGION_PROGRESS},
                           {"REGION_COUNT", M_SIGNAL_REGION_COUNT},
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
        , m_is_batch_read(false)
        , M_NUM_RANK(m_profile_sample->per_rank_region_id().size())
        , m_per_rank_progress(M_NUM_RANK, NAN)
        , m_per_rank_runtime(M_NUM_RANK, NAN)
        , m_per_rank_count(M_NUM_RANK, 0)
        , m_thread_progress(topo.num_domain(GEOPM_DOMAIN_CPU), NAN)
        , m_epoch_runtime_mpi(M_NUM_RANK, 0.0)
        , m_epoch_runtime_ignore(M_NUM_RANK, 0.0)
        , m_epoch_runtime(M_NUM_RANK, 0.0)
        , m_epoch_count(M_NUM_RANK, 0.0)
        , m_cpu_rank(m_profile_sample->cpu_rank())
    {
        topo.define_cpu_mpi_rank_map(m_cpu_rank);
        // now topo.num_domain(GEOPM_DOMAIN_MPI_RANK) will not throw
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
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = GEOPM_DOMAIN_MPI_RANK;
        }
        return result;
    }

    int ProfileIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
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
            // Runtime and count signals require region hash signal to be sampled
            if (signal_type == M_SIGNAL_RUNTIME ||
                signal_type == M_SIGNAL_REGION_COUNT) {
                m_do_read[M_SIGNAL_REGION_HASH] = true;
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
        if (m_do_read[M_SIGNAL_REGION_HASH] ||
            m_do_read[M_SIGNAL_REGION_HINT]) {
            m_per_rank_region_id = m_profile_sample->per_rank_region_id();
        }
        if (m_do_read[M_SIGNAL_REGION_PROGRESS]) {
            struct geopm_time_s read_time;
            geopm_time(&read_time);
            m_per_rank_progress = m_profile_sample->per_rank_progress(read_time);
        }
        if (m_do_read[M_SIGNAL_REGION_COUNT]) {
            m_per_rank_count = m_profile_sample->per_rank_count();
        }
        if (m_do_read[M_SIGNAL_THREAD_PROGRESS]) {
            m_thread_progress = m_profile_sample->per_cpu_thread_progress();
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME]) {
            m_epoch_runtime = m_epoch_regulator.last_epoch_runtime();
        }
        if (m_do_read[M_SIGNAL_EPOCH_COUNT]) {
            m_epoch_count = m_epoch_regulator.epoch_count();
        }
        if (m_do_read[M_SIGNAL_RUNTIME]) {
            // look up the region for each cpu and cache the per-cpu runtimes for that region
            std::map<uint64_t, std::vector<double> > cache;
            for (const auto &rid : m_per_rank_region_id) {
                // add runtimes for each region if not already present
                auto it = cache.find(rid);
                if (it == cache.end()) {
                    cache.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(rid),
                                  std::forward_as_tuple(m_profile_sample->per_rank_runtime(rid)));
                }
            }
            // look up the last runtime for a cpu given its current region
            // we assume ranks don't move between cpus
            for (size_t cpu = 0; cpu < m_per_rank_runtime.size(); ++cpu) {
                m_per_rank_runtime[cpu] = cache.at(m_per_rank_region_id[cpu])[cpu];
            }
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME_MPI]) {
            m_epoch_runtime_mpi = m_epoch_regulator.last_epoch_runtime_mpi();
        }
        if (m_do_read[M_SIGNAL_EPOCH_RUNTIME_IGNORE]) {
            m_epoch_runtime_ignore = m_epoch_regulator.last_epoch_runtime_ignore();
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

        int domain_idx = m_active_signal[signal_idx].domain_idx;
        switch (m_active_signal[signal_idx].signal_type) {
            case M_SIGNAL_REGION_HASH:
                result = geopm_region_id_hash(m_per_rank_region_id[domain_idx]);
                break;
            case M_SIGNAL_REGION_HINT:
                result = geopm_region_id_hint(m_per_rank_region_id[domain_idx]);
                break;
            case M_SIGNAL_REGION_PROGRESS:
                result = m_per_rank_progress[domain_idx];
                break;
            case M_SIGNAL_REGION_COUNT:
                result = m_per_rank_count[domain_idx];
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_thread_progress[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME:
                result = m_epoch_runtime[domain_idx];
                break;
            case M_SIGNAL_EPOCH_COUNT:
                result = m_epoch_count[domain_idx];
                break;
            case M_SIGNAL_RUNTIME:
                result = m_per_rank_runtime[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_MPI:
                result = m_epoch_runtime_mpi[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_IGNORE:
                result = m_epoch_runtime_ignore[domain_idx];
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
        struct geopm_time_s read_time;
        uint64_t region_id;
        double result = NAN;
        switch (signal_type) {
            case M_SIGNAL_REGION_HASH:
                result = geopm_region_id_hash(m_profile_sample->per_rank_region_id()[domain_idx]);
                break;
            case M_SIGNAL_REGION_HINT:
                result = geopm_region_id_hint(m_profile_sample->per_rank_region_id()[domain_idx]);
                break;
            case M_SIGNAL_REGION_PROGRESS:
                geopm_time(&read_time);
                result = m_profile_sample->per_rank_progress(read_time)[domain_idx];
                break;
            case M_SIGNAL_REGION_COUNT:
                result = m_profile_sample->per_rank_count()[domain_idx];
                break;
            case M_SIGNAL_THREAD_PROGRESS:
                result = m_profile_sample->per_cpu_thread_progress()[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME:
                result = m_epoch_regulator.last_epoch_runtime()[domain_idx];
                break;
            case M_SIGNAL_EPOCH_COUNT:
                result = m_epoch_regulator.epoch_count()[domain_idx];
                break;
            case M_SIGNAL_RUNTIME:
                region_id = m_profile_sample->per_rank_region_id()[domain_idx];
                result = m_profile_sample->per_rank_runtime(region_id)[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_MPI:
                result = m_epoch_regulator.last_epoch_runtime_mpi()[domain_idx];
                break;
            case M_SIGNAL_EPOCH_RUNTIME_IGNORE:
                result = m_epoch_regulator.last_epoch_runtime_ignore()[domain_idx];
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
            {"REGION_HASH", Agg::region_hash},
            {"PROFILE::REGION_HASH", Agg::region_hash},
            {"REGION_HINT", Agg::region_hint},
            {"PROFILE::REGION_HINT", Agg::region_hint},
            {"REGION_COUNT", Agg::min},
            {"PROFILE::REGION_COUNT", Agg::min},
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

    std::function<std::string(double)> ProfileIOGroup::format_function(const std::string &signal_name) const
    {
       static const std::map<std::string, std::function<std::string(double)> > fmt_map {
            {"REGION_RUNTIME", string_format_double},
            {"REGION_COUNT", string_format_integer},
            {"PROFILE::REGION_RUNTIME", string_format_double},
            {"REGION_PROGRESS", string_format_float},
            {"PROFILE::REGION_COUNT", string_format_integer},
            {"PROFILE::REGION_PROGRESS", string_format_float},
            {"REGION_THREAD_PROGRESS", string_format_float},
            {"PROFILE::REGION_THREAD_PROGRESS", string_format_float},
            {"REGION_HASH", string_format_hex},
            {"PROFILE::REGION_HASH", string_format_hex},
            {"REGION_HINT", string_format_hex},
            {"PROFILE::REGION_HINT", string_format_hex},
            {"EPOCH_RUNTIME", string_format_double},
            {"PROFILE::EPOCH_RUNTIME", string_format_double},
            {"EPOCH_ENERGY", string_format_double},
            {"PROFILE::EPOCH_ENERGY", string_format_double},
            {"EPOCH_COUNT", string_format_integer},
            {"PROFILE::EPOCH_COUNT", string_format_integer},
            {"EPOCH_RUNTIME_MPI", string_format_double},
            {"PROFILE::EPOCH_RUNTIME_MPI", string_format_double},
            {"EPOCH_RUNTIME_IGNORE", string_format_double},
            {"PROFILE::EPOCH_RUNTIME_IGNORE", string_format_double}
        };
        auto it = fmt_map.find(signal_name);
        if (it == fmt_map.end()) {
            throw Exception("ProfileIOGroup::format_function(): unknown how to format \"" + signal_name + "\"",
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
        if (domain_type == GEOPM_DOMAIN_MPI_RANK) {
            int rank_idx = domain_idx;
            if (rank_idx < 0 || rank_idx >= m_platform_topo.num_domain(GEOPM_DOMAIN_MPI_RANK)) {
                throw Exception("ProfileIOGroup::check_signal(): domain index out of range",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        else {
            throw Exception("ProfileIOGroup::check_signal(): domain type invalid",
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
