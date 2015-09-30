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

#include <string>
#include <inttypes.h>
#include <cpuid.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdexcept>
#include <sstream>

#include "geopm_policy_message.h"

#include "GlobalPolicy.hpp"
#include "StaticModeController.hpp"
#include "IVTPlatformImp.hpp"
#include "HSXPlatformImp.hpp"

#ifdef __cplusplus
extern "C" {
#endif
int staticpm_ctl_enforce(char* path)
{
    int err = 0;
    int mode = 0;
    geopm::GlobalPolicy policy(std::string(path), std::string(""));
    geopm::StaticModeController controller;

    try {
        policy.read();
        mode = policy.mode();
        switch (mode) {
            case GEOPM_MODE_TDP_BALANCE_STATIC:
                controller.tdp_limit(policy.percent_tdp());
                break;
            case GEOPM_MODE_FREQ_UNIFORM_STATIC:
                controller.manual_frequency(policy.frequency_mhz(), 0, GEOPM_FLAGS_BIG_CPU_TOPOLOGY_SCATTER);
                break;
            case GEOPM_MODE_FREQ_HYBRID_STATIC:
                controller.manual_frequency(policy.frequency_mhz(), policy.num_max_perf(), policy.affinity());
                break;
            default:
                std::cerr << "unsupported enforcement mode\n";
                return EINVAL;
        };
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }
    return err;
}

int staticpm_ctl_save(char *path)
{
    int err = 0;
    geopm::StaticModeController controller;

    try {
        controller.save_msr_state(path);
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }

    return err;
}

int staticpm_ctl_restore(char *path)
{
    int err = 0;
    geopm::StaticModeController controller;

    try {
        controller.restore_msr_state(path);
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }

    return err;
}
#ifdef __cplusplus
}
#endif

namespace geopm
{
    static const uint64_t PKG_POWER_LIMIT_MASK_MAGIC  = 0x0007800000078000ul;

    StaticModeController::StaticModeController()
        : m_platform(NULL) {}

    StaticModeController::~StaticModeController() {}

    void StaticModeController::tdp_limit(int percentage)
    {
        init_platform();

        //Get the TDP for each socket and set it's power limit to match
        double tdp = 0.0;
        double power_units = pow(2, (double)((m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT") >> 0) & 0xF));
        int num_packages = m_platform->get_num_package();
        int64_t pkg_lim, pkg_magic;

        for (int i = 0; i <  num_packages; i++) {
            tdp = ((double)(m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO") & 0x3fff)) / power_units;
            tdp *= ((double)percentage * 0.01);
            pkg_lim = (int64_t)(tdp * tdp);
            pkg_magic = pkg_lim | (pkg_lim << 32) | PKG_POWER_LIMIT_MASK_MAGIC;
            m_platform->write_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", pkg_magic);
        }
    }

    void StaticModeController::manual_frequency(int frequency, int num_cpu_max_perf, int affinity)
    {
        init_platform();

        //Set the frequency for each cpu
        int64_t freq_perc;
        bool small = false;
        int num_logical_cpus = m_platform->get_num_cpu();
        int num_hyperthreads = m_platform->get_num_hyperthreads();
        int num_real_cpus = num_logical_cpus / num_hyperthreads;
        int num_packages = m_platform->get_num_package();
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
                m_platform->write_msr(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
            }
            small = false;
        }
    }

    void StaticModeController::save_msr_state(char *path)
    {
        init_platform();

        uint64_t msr_val;
        int niter = m_platform->get_num_package();
        std::ofstream restore_file;

        restore_file.open(path);

        //per package state
        for (int i = 0; i < niter; i++) {
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_platform->get_msr_offset("PKG_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_platform->get_msr_offset("PP0_POWER_LIMIT") << ":" << msr_val << "\n";
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_platform->get_msr_offset("DRAM_POWER_LIMIT") << ":" << msr_val << "\n";
        }

        niter = m_platform->get_num_cpu() / m_platform->get_num_hyperthreads();

        //per cpu state
        for (int i = 0; i < niter; i++) {
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_FIXED_CTR_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_platform->get_msr_offset("PERF_FIXED_CTR_CTR") << ":" << msr_val << "\n";
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_platform->get_msr_offset("PERF_GLOBAL_CTR") << ":" << msr_val << "\n";
            msr_val = m_platform->read_msr(GEOPM_DOMAIN_CPU, i, "PERF_GLOBAL_OVF_CTRL");
            restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_platform->get_msr_offset("PERF_GLOBAL_OVF_CTR") << ":" << msr_val << "\n";

        }

        restore_file.close();
    }

    void StaticModeController::restore_msr_state(char *path)
    {
        init_platform();
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
                m_platform->write_msr(vals[0], vals[1], vals[2], vals[3]);
            }
            else {
                throw std::runtime_error("error detected in restore file. Could not restore msr states");
            }
            vals.clear();
        }
        restore_file.close();
        remove("static_reg_restore.txt");
    }

    void StaticModeController::init_platform(void)
    {
        int cpuid = read_cpuid();
        m_platform = NULL;


        switch (cpuid) {
            case 0x62d: //Sandy Bridge E
            case 0x63e: //Ivy Bridge E
                m_platform = new IVTPlatformImp();
                break;
            case 0x63f: //Haswell E
                m_platform = new HSXPlatformImp();
                break;
            default:
                throw std::invalid_argument("no module found to support current platform");
        }

    }

    int StaticModeController::read_cpuid()
    {
        uint32_t key = 1; //processor features
        uint32_t proc_info = 0;
        uint32_t model;
        uint32_t family;
        uint32_t ext_model;
        uint32_t ext_family;
        uint32_t ebx, ecx, edx;
        const uint32_t model_mask = 0xF0;
        const uint32_t family_mask = 0xF00;
        const uint32_t extended_model_mask = 0xF0000;
        const uint32_t extended_family_mask = 0xFF00000;

        __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);

        model = (proc_info & model_mask) >> 4;
        family = (proc_info & family_mask) >> 8;
        ext_model = (proc_info & extended_model_mask) >> 16;
        ext_family = (proc_info & extended_family_mask)>> 20;

        if (family == 6) {
            model+=(ext_model << 4);
        }
        else if (family == 15) {
            model+=(ext_model << 4);
            family+=ext_family;
        }

        return ((family << 8) + model);
    }
}
