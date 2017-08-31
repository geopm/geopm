/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <set>
#include <string>
#include <inttypes.h>
#include <cpuid.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdexcept>
#include <float.h>

#include "Exception.hpp"
#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "PlatformImp.hpp"
#include "geopm_message.h"
#include "geopm_policy.h"
#include "config.h"

extern "C"
{
    int geopm_platform_msr_save(const char *path)
    {
        int err = 0;
        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform("rapl", true);
            platform->save_msr_state(path);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_platform_msr_restore(const char *path)
    {
        int err = 0;

        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform("rapl", true);
            platform->restore_msr_state(path);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }

    int geopm_platform_msr_whitelist(FILE *file_desc)
    {
        int err = 0;
        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform("rapl", false);

            platform->write_msr_whitelist(file_desc);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }

        return err;
    }
}

namespace geopm
{
    Platform::Platform()
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(GEOPM_CONTROL_DOMAIN_POWER)
        , m_num_energy_domain(0)
        , m_num_counter_domain(0)
        , m_num_rank(0)
    {

    }

    Platform::Platform(int control_domain_type)
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(control_domain_type)
        , m_num_energy_domain(0)
        , m_num_counter_domain(0)
        , m_num_rank(0)
    {

    }

    Platform::~Platform()
    {

    }

    void Platform::set_implementation(PlatformImp* platform_imp, bool do_initialize)
    {
        m_imp = platform_imp;
        if (do_initialize) {
            m_imp->initialize();
            initialize();
        }
    }

    void Platform::name(std::string &plat_name) const
    {
        if (m_imp == NULL) {
            throw Exception("Platform implementation is missing", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        plat_name = m_imp->platform_name();
    }

    int Platform::num_domain(void) const
    {
        return m_imp->num_energy_signal();
    }

    const PlatformTopology *Platform::topology(void) const
    {
        return m_imp->topology();
    }

    void Platform::transform_rank_data(uint64_t region_id, const struct geopm_time_s &aligned_time, const std::vector<double> &aligned_data, std::vector<struct geopm_telemetry_message_s> &telemetry)
    {
        const int NUM_RANK_SIGNAL = 2;
        int num_package = m_imp->num_package();
        int num_cpu = m_imp->num_logical_cpu();
        int num_platform_signal = m_imp->num_energy_signal() + m_imp->num_counter_signal();
        /// @todo assumes domain of control is the package
        std::vector<double> runtime(num_package);
        std::vector<double> min_progress(num_package);
        std::vector<double> max_progress(num_package);

        std::fill(runtime.begin(), runtime.end(), -DBL_MAX);
        std::fill(min_progress.begin(), min_progress.end(), DBL_MAX);
        std::fill(max_progress.begin(), max_progress.end(), -DBL_MAX);

        int num_cpu_per_package = num_cpu / num_package;
        if (m_imp->power_control_domain() == GEOPM_DOMAIN_PACKAGE) {
            int rank_offset = num_package * num_platform_signal;
            int rank_id = 0;
            for (size_t i = rank_offset;  i < aligned_data.size(); i += NUM_RANK_SIGNAL)  {
                for (auto it = m_rank_cpu[rank_id].begin(); it != m_rank_cpu[rank_id].end(); ++it) {
                    if (aligned_data[i + 1] != -1.0) {
                        // Find minimum progress for any rank on the package
                        if (aligned_data[i] < min_progress[(*it) / num_cpu_per_package]) {
                            min_progress[(*it) / num_cpu_per_package] = aligned_data[i];
                        }
                        // Find maximum progress for any rank on the package
                        if (aligned_data[i] > max_progress[(*it) / num_cpu_per_package]) {
                            max_progress[(*it) / num_cpu_per_package] = aligned_data[i];
                        }
                        // Find maximum runtime for any rank on the package
                        if (aligned_data[i + 1] > runtime[(*it) / num_cpu_per_package]) {
                            runtime[(*it) / num_cpu_per_package] = aligned_data[i + 1];
                        }
                    }
                }
                ++rank_id;
            }
            // Insert platform signals
            for (int i = 0; i < rank_offset; ++i) {
                int domain_idx = i / num_platform_signal;
                int signal_idx = i % num_platform_signal;
                telemetry[domain_idx].signal[signal_idx] = aligned_data[domain_idx * num_platform_signal + signal_idx];
            }
            // Insert application signals
            int domain_idx = 0;
            for (int i = 0; i < num_package * NUM_RANK_SIGNAL; i += NUM_RANK_SIGNAL) {
                // Do not drop a region exit
                if (max_progress[domain_idx] == 1.0) {
                    telemetry[domain_idx].signal[num_platform_signal] = 1.0;
                }
                else {
                    telemetry[domain_idx].signal[num_platform_signal] = min_progress[domain_idx] == DBL_MAX ? 0.0 : min_progress[domain_idx];
                }
                telemetry[domain_idx].signal[num_platform_signal + 1] = runtime[domain_idx] == -DBL_MAX ? -1.0 : runtime[domain_idx];
                ++domain_idx;
            }
            // Insert region and timestamp
            for (int i = 0; i < num_package; ++i) {
                telemetry[i].region_id = region_id;
                telemetry[i].timestamp = aligned_time;
            }
        }
    }

    void Platform::init_transform(const std::vector<int> &cpu_rank)
    {
        std::set<int> rank_set;
        for (auto it = cpu_rank.begin(); it != cpu_rank.end(); ++it) {
            rank_set.insert(*it);
        }
        m_num_rank = rank_set.size();
        int i = 0;
        std::map<int, int> rank_map;
        for (auto it = rank_set.begin(); it != rank_set.end(); ++it) {
            rank_map.insert(std::pair<int, int>(*it, i));
            ++i;
        }
        m_rank_cpu.resize(m_num_rank);
        for (i = 0; i < (int)cpu_rank.size(); ++i) {
            m_rank_cpu[rank_map.find(cpu_rank[i])->second].push_back(i);
        }
    }

    int Platform::num_control_domain(void) const
    {
        return (topology()->num_domain(m_imp->power_control_domain()));
    }

    void Platform::tdp_limit(double percentage) const
    {
        //Get the TDP for each socket and set its power limit to match
        int packages = m_imp->num_package();
        double tdp = m_imp->package_tdp();
        double pkg_lim = tdp * (percentage * 0.01);
        for (int i = 0; i < packages; i++) {
            m_imp->write_control(m_imp->power_control_domain(), i,  GEOPM_TELEMETRY_TYPE_PKG_ENERGY, pkg_lim);
        }
    }

    void Platform::manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const
    {
        //Set the frequency for each cpu
        int64_t freq_perc;
        bool small = false;
        int num_logical_cpus = m_imp->num_logical_cpu();
        int num_real_cpus = m_imp->num_hw_cpu();
        int packages = m_imp->num_package();
        int num_cpus_per_package = num_real_cpus / packages;
        int num_small_cores_per_package = num_cpus_per_package - (num_cpu_max_perf / packages);

        if (num_cpu_max_perf >= num_real_cpus) {
            throw Exception("requested number of max perf cpus is greater than controllable number of frequency domains on the platform",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (int i = 0; i < num_logical_cpus; i++) {
            int real_cpu = i % num_real_cpus;
            if (affinity == GEOPM_POLICY_AFFINITY_SCATTER && num_cpu_max_perf > 0) {
                if ((real_cpu % num_cpus_per_package) < num_small_cores_per_package) {
                    small = true;
                }
            }
            else if (affinity == GEOPM_POLICY_AFFINITY_COMPACT && num_cpu_max_perf > 0) {
                if (real_cpu < (num_real_cpus - num_cpu_max_perf)) {
                    small = true;
                }
            }
            else {
                small = true;
            }
            if (small) {
                freq_perc = ((int64_t)(frequency * 0.01) << 8) & 0xffff;
                m_imp->msr_write(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
            }
            small = false;
        }
    }

    void Platform::save_msr_state(const char *path) const
    {
        m_imp->save_msr_state(path);
    }

    void Platform::restore_msr_state(const char *path) const
    {
        m_imp->restore_msr_state(path);
    }

    void Platform::write_msr_whitelist(FILE *file_desc) const
    {
        if (file_desc == NULL) {
            throw Exception("Platform(): file descriptor is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_imp->whitelist(file_desc);
    }

    void Platform::revert_msr_state(void) const
    {
        m_imp->revert_msr_state();
    }

    double Platform::control_latency_ms(void) const
    {
        return m_imp->control_latency_ms();
    }

    double Platform::throttle_limit_mhz(void) const
    {
        return m_imp->throttle_limit_mhz();
    }

    bool Platform::is_updated(void)
    {
        return m_imp->is_updated();
    }
}
