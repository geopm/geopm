/*
 * Copyright (c) 2015, Intel Corporation
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

#include "Platform.hpp"
#include "PlatformFactory.hpp"
#include "geopm_policy_message.h"

extern "C"
{
    int geopm_platform_msr_save(const char *path)
    {
        int err = 0;
        geopm::PlatformFactory platform_factory;
        geopm::Platform *platform = platform_factory.platform(0);
        try {
            platform->save_msr_state(path);
        }
        catch (std::exception ex) {
            std::cerr << ex.what();
            err = -1;
        }

        return err;
    }

    int geopm_platform_msr_restore(const char *path)
    {
        int err = 0;
        geopm::PlatformFactory platform_factory;
        geopm::Platform *platform = platform_factory.platform(0);

        try {
            platform->restore_msr_state(path);
        }
        catch (std::exception ex) {
            std::cerr << ex.what();
            err = -1;
        }

        return err;
    }
}

namespace geopm
{
    static const uint64_t PKG_POWER_LIMIT_MASK_MAGIC  = 0x0007800000078000ul;

    Platform::Platform()
        : m_imp(NULL),
          m_level(-1) {}

    Platform::~Platform()
    {
        std::set <PowerModel *> power_model_set;
        for (auto it = m_power_model.begin(); it != m_power_model.end(); ++it) {
            power_model_set.insert(it->second);
        }
        for (auto it = power_model_set.begin(); it != power_model_set.end(); ++it) {
            delete *it;
        }
    }

    void Platform::set_implementation(PlatformImp* platform_imp)
    {
        PowerModel *power_model = new PowerModel();
        m_imp = platform_imp;
        m_imp->initialize_msrs();
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_PACKAGE_UNCORE, power_model));
        m_power_model.insert(std::pair <int, PowerModel*>(GEOPM_DOMAIN_BOARD_MEMORY, power_model));
    }

    std::string Platform::name(void) const
    {
        if (m_imp == NULL) {
            throw std::runtime_error("Platform implementation is missing");
        }
        return m_imp->get_platform_name();
    }

    void Platform::buffer_index(hwloc_obj_t domain,
                                const std::vector <std::string> &signal_names,
                                std::vector <int> &buffer_index) const
    {
        /* FIXME need to figure out how to implement this function */
        throw std::runtime_error("Platform does not support buffer_index() method\n");
    }

    int Platform::level(void) const
    {
        return m_level;
    }

    void Platform::phase_begin(Phase *phase)
    {
        m_cur_phase = phase;
    }

    void Platform::phase_end(void)
    {
        m_cur_phase = NULL;
    }

    int Platform::num_domain(void) const
    {
        return m_num_domains;
    }

    void Platform::domain_index(int domain_type, std::vector <int> &domain_index) const
    {
        // FIXME
        throw(std::runtime_error("Platform does not support domain_index() method"));
    }

    void Platform::observe(const std::vector <struct sample_message_s> &sample) const
    {
        //check if we are in unmarked code
        if (m_cur_phase == NULL) {
            return;
        }
    }

    Phase *Platform::cur_phase(void) const
    {
        return m_cur_phase;
    }

    const PlatformTopology Platform::topology(void) const
    {
        return m_imp->topology();
    }

    PowerModel *Platform::power_model(int domain_type) const
    {
        auto model =  m_power_model.find(domain_type);
        if (model == m_power_model.end()) {
            throw std::invalid_argument("No PowerModel found for given domain_type\n");
        }
        return model->second;
    }

    void Platform::tdp_limit(int percentage) const
    {
        //Get the TDP for each socket and set it's power limit to match
        double tdp = 0.0;
        double power_units = pow(2, (double)((m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT") >> 0) & 0xF));
        int num_packages = m_imp->get_num_package();
        int64_t pkg_lim, pkg_magic;

        for (int i = 0; i <  num_packages; i++) {
            tdp = ((double)(m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO") & 0x3fff)) / power_units;
            tdp *= ((double)percentage * 0.01);
            pkg_lim = (int64_t)(tdp * tdp);
            pkg_magic = pkg_lim | (pkg_lim << 32) | PKG_POWER_LIMIT_MASK_MAGIC;
            m_imp->write_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", pkg_magic);
        }
    }

    void Platform::manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const
    {
        //Set the frequency for each cpu
        int64_t freq_perc;
        bool small = false;
        int num_logical_cpus = m_imp->get_num_cpu();
        int num_hyperthreads = m_imp->get_num_hyperthreads();
        int num_real_cpus = num_logical_cpus / num_hyperthreads;
        int num_packages = m_imp->get_num_package();
        int num_cpus_per_package = num_real_cpus / num_packages;
        int num_small_cores_per_package = num_cpus_per_package - (num_cpu_max_perf / num_packages);

        if (num_cpu_max_perf >= num_real_cpus) {
            throw std::runtime_error("requested number of max perf cpus is greater than controllable number of frequency domains on the platform");
        }

        for (int i = 0; i < num_logical_cpus; i++) {
            int real_cpu = i % num_real_cpus;
            if (affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER) {
                int package = real_cpu % num_cpus_per_package;
                int extra = num_cpu_max_perf % num_packages;
                extra = (package < extra) ? 1 : 0;
                int package_start = package * num_cpus_per_package;
                int small_cpu_end = package_start + num_small_cores_per_package + extra;
                if (real_cpu >= package_start && real_cpu < small_cpu_end) {
                    small = true;
                }
            }
            else if (affinity == GEOPM_FLAGS_BIG_CPU_TOPOLOGY_COMPACT) {
                if (real_cpu > (num_cpu_max_perf % num_hyperthreads)) {
                    small = true;
                }
            }
            else {
                small = true;
            }
            if (small) {
                freq_perc = ((int64_t)(frequency * 10.01) << 8) & 0xffff;
                m_imp->write_msr(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
            }
            small = false;
        }
    }

    void Platform::save_msr_state(const char *path) const
    {
        uint64_t msr_val;
        int niter = m_imp->get_num_package();
        std::ofstream restore_file;

        restore_file.open(path);

        //per package state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->get_msr_offset("PKG_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->get_msr_offset("PP0_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_imp->get_msr_offset("DRAM_POWER_LIMIT") << ":" << msr_val << "\n";
        }

        niter = m_imp->get_num_cpu() / m_imp->get_num_hyperthreads();

        //per cpu state
        for (int i = 0; i < niter; i++) {
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_FIXED_CTR_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->get_msr_offset("PERF_FIXED_CTR_CTR") << ":" << msr_val << "\n";
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->get_msr_offset("PERF_GLOBAL_CTR") << ":" << msr_val << "\n";
            msr_val = m_imp->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_OVF_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_imp->get_msr_offset("PERF_GLOBAL_OVF_CTR") << ":" << msr_val << "\n";

        }

        restore_file.close();
    }

    void Platform::restore_msr_state(const char *path) const
    {
        std::ifstream restore_file;
        std::string line;
        std::vector<int64_t> vals;
        std::string item;
        restore_file.open(path);
        while (std::getline(restore_file,line)) {
            std::stringstream ss(line);
            while (std::getline(ss, item, ':')) {
                vals.push_back((int64_t)atol(item.c_str()));
            }
            if (vals.size() == 4) {
                m_imp->write_msr(vals[0], vals[1], vals[2], vals[3]);
            }
            else {
                throw std::runtime_error("error detected in restore file. Could not restore msr states");
            }
            vals.clear();
        }
        restore_file.close();
        remove("static_reg_restore.txt");
    }
}
