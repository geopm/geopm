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

#include "StaticModeController.hpp"
#include "IVTPlatformImp.hpp"
#include "HSXPlatformImp.hpp"

#ifdef __cplusplus
extern "C" {
#endif
int staticpm_ctl_run_tdp(int mode, float percentage)
{
    int err = 0;
    geopm::StaticModeController controller;

    try {
        controller.tdp_limit(percentage);
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }
    return err;
}

int staticpm_ctl_run_fixed(int mode, float frequency)
{
    int err = 0;
    geopm::StaticModeController controller;
    int mask = 0;

    try {
        controller.manual_frequency(frequency, 0, (int*)&mask);
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }
    return err;
}

int staticpm_ctl_run_hybrid(int mode, float frequency, int mask_size, int* mask)
{
    int err = 0;
    geopm::StaticModeController controller;

    try {
        controller.manual_frequency(frequency, mask_size, mask);
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }

    return err;
}

int staticpm_ctl_restore()
{
    int err = 0;
    geopm::StaticModeController controller;

    try {
        controller.restore_state();
    }
    catch (std::exception ex) {
        std::cerr << ex.what();
        err = -1;
    }
    return err;
}
#ifdef _cplusplus
}
#endif

namespace geopm
{
    static const uint64_t PKG_POWER_LIMIT_MASK_MAGIC  = 0x0007800000078000ul;

    StaticModeController::StaticModeController()
        : m_platform(NULL) {}

    StaticModeController::~StaticModeController() {}

    void StaticModeController::tdp_limit(float percentage)
    {
        init_platform();
        std::ofstream restore_file;
        restore_file.open("static_reg_restore.txt");
        printf("perc2 = %f\n", percentage);

        //Get the TDP for each socket and set it's power limit to match
        double tdp = 0.0;
        double power_units = pow(2, (double)((m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT") >> 0) & 0xF));
        int num_packages = m_platform->get_num_package();
        int64_t pkg_lim, pkg_magic, old_val;;

        for (int i = 0; i <  num_packages; i++) {
            tdp = ((double)(m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO") & 0x3fff)) / power_units;
            tdp *= (percentage * 0.01);
            pkg_lim = (int64_t)(tdp * tdp);
            pkg_magic = pkg_lim | (pkg_lim << 32) | PKG_POWER_LIMIT_MASK_MAGIC;
            old_val = m_platform->read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT");
            restore_file << GEOPM_DOMAIN_PACKAGE << ":" << i << ":" << m_platform->get_msr_offset("PKG_POWER_LIMIT")
                         << ":" << old_val << "\n";
            m_platform->write_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", pkg_magic);
        }
        restore_file.close();
    }

    void StaticModeController::manual_frequency(float frequency, int mask_size, int *mask)
    {
        init_platform();
        std::ofstream restore_file;
        restore_file.open("static_reg_restore.txt");
        printf("freq2 = %f\n", frequency);
        //Set the frequency for each cpu
        int64_t freq_perc;
        int num_cpus = m_platform->get_num_cpu();
        if (m_platform->is_hyperthread_enabled()) {
            num_cpus/=2;
        }
        for (int i = 0; i < num_cpus; i++) {
            int masknum = i/32;
            int bitnum = i%32;
            int temp_mask = 1 << bitnum;
            printf("\n\ncpu %d\n", i);
            printf("masknum = %d, bitnum = %d\n", masknum, bitnum);
            printf("mask[%d] = %x, tempmask = %x\n",masknum,mask[masknum],temp_mask);
            if ((mask[masknum] & temp_mask) == 0) {
                freq_perc = m_platform->read_msr(GEOPM_DOMAIN_CPU, i, "IA32_PERF_STATUS") & 0xffff;
                restore_file << GEOPM_DOMAIN_CPU << ":" << i << ":" << m_platform->get_msr_offset("PKG_POWER_LIMIT")
                             << ":" << freq_perc << "\n";
                printf("perf status = %d:%f\n", freq_perc, (float)(freq_perc >> 8) * 0.1);
                freq_perc = ((int64_t)(frequency * 10.01) << 8) & 0xffff;
                m_platform->write_msr(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
                freq_perc = m_platform->read_msr(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL") & 0xffff;
                printf("frequency = %f, divided = %d, shifted = %d\n", frequency, (int)(frequency*10.01), ((int(frequency*10))<<8));
                printf("perf control = %d:%f\n", freq_perc, (float)(freq_perc >> 8) * 0.1);
            }
        }
        restore_file.close();

    }

    void StaticModeController::restore_state(void)
    {
        init_platform();
        std::ifstream restore_file;
        std::string line;
        std::vector<int64_t> vals;
        std::string item;
        restore_file.open("static_reg_restore.txt");
        while (std::getline(restore_file,line)) {
            std::stringstream ss(line);
            while (std::getline(ss, item, ':')) {
                vals.push_back((int64_t)atol(item.c_str()));
            }
            if(vals.size() == 4) {
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
        m_platform == NULL;


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

        //Not sure if this is the correct call
        __get_cpuid(key, &proc_info, &ebx, &ecx, &edx);
        //Commenting out assembly which works only on x86_64
        //__asm__("cpuid"
        //        :"=a"(proc_info)
        //        :"0"(key)
        //        :"%ebx","%ecx","%edx");

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
