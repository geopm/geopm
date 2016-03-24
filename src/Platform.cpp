/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <sstream>

#include "geopm_error.h"
#include "Exception.hpp"
#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "geopm_message.h"

extern "C"
{
    int geopm_platform_msr_save(const char *path)
    {
        int err = 0;
        try {
            geopm::PlatformFactory platform_factory;
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));
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
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));
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
            geopm::Platform *platform = platform_factory.platform(std::string("rapl"));

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
    static const uint64_t PKG_POWER_LIMIT_MASK_MAGIC  = 0x0007800000078000ul;

    Platform::Platform()
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(GEOPM_CONTROL_DOMAIN_POWER)
    {

    }

    Platform::Platform(int control_domain_type)
        : m_imp(NULL)
        , m_num_domain(0)
        , m_control_domain_type(control_domain_type)
        , m_num_rank(0)
    {

    }

    Platform::~Platform()
    {

    }

    void Platform::set_implementation(PlatformImp* platform_imp)
    {
        m_imp = platform_imp;
        m_imp->initialize();
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
        return m_imp->num_package_signal();
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
        int num_platform_signal = m_imp->num_package_signal() + m_imp->num_cpu_signal();
        std::vector<double> runtime(num_package);
        std::vector<double> min_progress(num_package);
        std::vector<double> max_progress(num_package);

        std::fill(runtime.begin(), runtime.end(), DBL_MIN);
        std::fill(min_progress.begin(), min_progress.end(), DBL_MAX);
        std::fill(max_progress.begin(), max_progress.end(), DBL_MIN);

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
                        if (aligned_data[i + 1] >  runtime[(*it) / num_cpu_per_package]) {
                            runtime[(*it) / num_cpu_per_package] = aligned_data[i + 1];
                        }
                    }
                }
                ++rank_id;
            }
            // Add platform signals
            for (int i = 0; i < rank_offset; ++i) {
                int domain = i / num_platform_signal;
                int signal = i % num_platform_signal;
                telemetry[domain].signal[signal] = aligned_data[domain * num_platform_signal + signal];
            }
            // Add application signals
            for (int i = 0; i < num_package * NUM_RANK_SIGNAL; i += NUM_RANK_SIGNAL) {;
                int domain = i / NUM_RANK_SIGNAL;
                // Do not drop a region exit
                if (max_progress[domain] == 1.0) {
                    telemetry[domain].signal[num_platform_signal] = 1.0;
                }
                else {
                    telemetry[domain].signal[num_platform_signal] = min_progress[domain] == DBL_MAX ? -1.0 : min_progress[domain];
                }
                telemetry[domain].signal[num_platform_signal + 1] = runtime[domain] == DBL_MIN ? 0.0 : runtime[domain];
            }
            // Add region and timestamp
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

    void Platform::tdp_limit(int percentage) const
    {
        //Get the TDP for each socket and set its power limit to match
        double tdp = 0.0;
        double power_units = pow(2, (double)((m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT") >> 0) & 0xF));
        int packages = m_imp->num_package();
        int64_t pkg_lim, pkg_magic;

        for (int i = 0; i <  packages; i++) {
            tdp = ((double)(m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO") & 0x3fff)) / power_units;
            tdp *= ((double)percentage * 0.01);
            pkg_lim = (int64_t)(tdp * tdp);
            pkg_magic = pkg_lim | (pkg_lim << 32) | PKG_POWER_LIMIT_MASK_MAGIC;
            m_imp->msr_write(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", pkg_magic);
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
            if (affinity == GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER && num_cpu_max_perf > 0) {
                if ((real_cpu % num_cpus_per_package) < num_small_cores_per_package) {
                    small = true;
                }
            }
            else if (affinity == GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT && num_cpu_max_perf > 0) {
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
        uint64_t msr_val;
        int niter = m_imp->num_package();
        std::ofstream restore_file;

        if (path == NULL) {
            throw Exception("Platform(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path);

        //per package state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("PKG_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("PP0_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->msr_offset("DRAM_POWER_LIMIT") << ":" << msr_val << "\n";
        }

        niter = m_imp->num_hw_cpu();

        //per cpu state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_FIXED_CTR_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_FIXED_CTR_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_GLOBAL_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_OVF_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("PERF_GLOBAL_OVF_CTRL") << ":" << msr_val << "\n";
            msr_val = m_imp->msr_read(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->msr_offset("IA32_PERF_CTL") << ":" << msr_val << "\n";

        }

        restore_file.close();
    }

    void Platform::restore_msr_state(const char *path) const
    {
        std::ifstream restore_file;
        std::string line;
        std::vector<int64_t> vals;
        std::string item;

        if (path == NULL) {
            throw Exception("Platform(): file path is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        restore_file.open(path, std::ios_base::in);

        while (std::getline(restore_file,line)) {
            std::stringstream ss(line);
            while (std::getline(ss, item, ':')) {
                vals.push_back((int64_t)atol(item.c_str()));
            }
            if (vals.size() == 4) {
                m_imp->msr_write(vals[0], vals[1], vals[2], vals[3]);
            }
            else {
                throw Exception("error detected in restore file. Could not restore msr states", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            vals.clear();
        }
        restore_file.close();
        remove(path);
    }

    void Platform::write_msr_whitelist(FILE *file_desc) const
    {
        if (file_desc == NULL) {
            throw Exception("Platform(): file descriptor is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_imp->whitelist(file_desc);
    }
}
