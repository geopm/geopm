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

#include <math.h>
#include <unistd.h>
#include "geopm_error.h"
#include "Exception.hpp"
#include "HSXPlatformImp.hpp"

namespace geopm
{
    static const int hsx_id = 0x63F;
    static const std::string hsx_model_name = "Haswell E";

    // common values
    static const unsigned int box_frz_en = 0x1 << 16;
    static const unsigned int box_frz    = 0x1 << 8;
    static const unsigned int ctr_en     = 0x1 << 22;
    static const unsigned int rst_ctrs   = 0x1 << 1;

    static const unsigned int llc_filter_mask     = 0x1F << 18;
    static const unsigned int llc_victims_ev_sel  = 0x37;
    static const unsigned int llc_victims_umask   = 0x7 << 8;

    static const unsigned int event_sel_0 = llc_victims_ev_sel;
    static const unsigned int umask_0     = llc_victims_umask;

    HSXPlatformImp::HSXPlatformImp()
        : m_energy_units(1.0)
        , m_power_units(1.0)
        , m_min_pkg_watts(1)
        , m_max_pkg_watts(100)
        , m_min_pp0_watts(1)
        , m_max_pp0_watts(100)
        , m_min_dram_watts(1)
        , m_max_dram_watts(100)
        , m_platform_id(hsx_id)
    {
    }

    HSXPlatformImp::~HSXPlatformImp()
    {
        while(m_cpu_file_descs.size()) {
            close(m_cpu_file_descs.back());
            m_cpu_file_descs.pop_back();
        }
    }

    bool HSXPlatformImp::model_supported(int platform_id)
    {
        m_platform_id = platform_id;
        return (platform_id == hsx_id);
    }

    std::string HSXPlatformImp::get_platform_name()
    {
        return hsx_model_name;
    }

    void HSXPlatformImp::initialize_msrs()
    {
        for (int i = 0; i < m_num_cpu; i++) {
            open_msr(i);
        }
        load_msr_offsets();
        rapl_init();
        cbo_counters_init();
        fixed_counters_init();
    }

    void HSXPlatformImp::reset_msrs()
    {
        rapl_reset();
        cbo_counters_reset();
        fixed_counters_reset();
    }

    void HSXPlatformImp::rapl_init()
    {
        uint64_t tmp;
        double energy_units, power_units;

        //Make sure units are consistent between packages
        tmp = read_msr(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT");
        energy_units = pow(0.5, (double)((tmp >> 8) & 0x1F));
        power_units = pow(2, (double)((tmp >> 0) & 0xF));

        for (int i = 1; i < m_num_package; i++) {
            tmp = read_msr(GEOPM_DOMAIN_PACKAGE, i, "RAPL_POWER_UNIT");
            double energy = pow(0.5, (double)((tmp >> 8) & 0x1F));
            double power = pow(2, (double)((tmp >> 0) & 0xF));
            if (energy != energy_units || power != power_units) {
                throw Exception("detected inconsistent power units among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        //Make sure bounds are consistent between packages
        tmp = read_msr(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_INFO");
        m_min_pkg_watts = ((double)((tmp >> 16) & 0x7fff)) / power_units;
        m_max_pkg_watts = ((double)((tmp >> 32) & 0x7fff)) / power_units;

        tmp = read_msr(GEOPM_DOMAIN_PACKAGE, 0, "DRAM_POWER_INFO");
        m_min_dram_watts = ((double)((tmp >> 16) & 0x7fff)) / power_units;
        m_max_dram_watts = ((double)((tmp >> 32) & 0x7fff)) / power_units;

        for (int i = 1; i < m_num_package; i++) {
            tmp = read_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO");
            double pkg_min = ((double)((tmp >> 16) & 0x7fff)) / power_units;
            double pkg_max = ((double)((tmp >> 32) & 0x7fff)) / power_units;
            if (pkg_min != m_min_pkg_watts || pkg_max != m_max_pkg_watts) {
                throw Exception("detected inconsistent power pkg bounds among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            tmp = read_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
            double dram_min = ((double)((tmp >> 16) & 0x7fff)) / power_units;
            double dram_max = ((double)((tmp >> 32) & 0x7fff)) / power_units;
            if (dram_min != m_min_dram_watts || dram_max != m_max_dram_watts) {
                throw Exception("detected inconsistent power dram bounds among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_min_pp0_watts = m_min_pkg_watts;
        m_max_pp0_watts = m_max_pkg_watts;

        rapl_reset();
    }

    void HSXPlatformImp::cbo_counters_init()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = msr_num/m_hyperthreads;
        for (int i = 0; i < msr_num; i++) {
            std::string msr_name("_MSR_PMON_CTL1");
            std::string box_msr_name("_MSR_PMON_BOX_CTL");
            std::string filter_msr_name("_MSR_PMON_BOX_FILTER");
            box_msr_name.insert(0, std::to_string(i));
            box_msr_name.insert(0, "C");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            filter_msr_name.insert(0, std::to_string(i));
            filter_msr_name.insert(0, "C");

            // enable freeze
            write_msr(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | box_frz_en);
            // freeze box
            write_msr(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | box_frz);
            write_msr(GEOPM_DOMAIN_CPU, i, msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, msr_name)
                      | ctr_en);
            write_msr(GEOPM_DOMAIN_CPU, i, filter_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, filter_msr_name)
                      | llc_filter_mask);
            // llc victims
            write_msr(GEOPM_DOMAIN_CPU, i, msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, msr_name)
                      | event_sel_0 | umask_0);
            // reset counters
            write_msr(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | rst_ctrs);
            //FIXME: is this needed???
            write_msr(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      & ~box_frz);
            // unfreeze box
            write_msr(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      & ~box_frz);
        }
    }

    void HSXPlatformImp::fixed_counters_init()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = msr_num/m_hyperthreads;
        for (int cpu = 0; cpu < msr_num; cpu++) {
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR_CTRL", 0x0333);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_CTRL", 0x70000000F);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void HSXPlatformImp::rapl_reset()
    {
        //clear power limits
        for (int i = 1; i < m_num_package; i++) {
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", 0x0);
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT", 0x0);
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT", 0x0);
        }

    }

    void HSXPlatformImp::cbo_counters_reset()
    {
        // FIXME: Instead of resetting, should we restore state?
        int msr_num = m_num_cpu/m_num_package;
        msr_num = msr_num/m_hyperthreads;
        for (int i = 0; i < msr_num; i++) {
            std::string msr_name("_MSR_PMON_BOX_CTL");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            // reset counters
            write_msr(GEOPM_DOMAIN_CPU, i, msr_name,
                      read_msr(GEOPM_DOMAIN_CPU, i, msr_name)
                      | rst_ctrs);
        }
    }

    void HSXPlatformImp::fixed_counters_reset()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = msr_num/m_hyperthreads;
        for (int cpu = 0; cpu < msr_num; cpu++) {
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR0", 0x0);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR1", 0x0);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR2", 0x0);
        }
    }

    void HSXPlatformImp::load_msr_offsets()
    {
        std::map<std::string,off_t> msr_map = {
            {"IA32_PERF_STATUS",        0x0198},
            {"IA32_PERF_CTL",           0x0199},
            {"RAPL_POWER_UNIT",         0x0606},
            {"PKG_POWER_LIMIT",         0x0610},
            {"PKG_ENERGY_STATUS",       0x0611},
            {"PKG_POWER_INFO",          0x0614},
            {"PP0_POWER_LIMIT",         0x0638},
            {"PP0_ENERGY_STATUS",       0x0639},
            {"DRAM_POWER_LIMIT",        0x0618},
            {"DRAM_ENERGY_STATUS",      0x0619},
            {"DRAM_PERF_STATUS",        0x061B},
            {"DRAM_POWER_INFO",         0x061C},
            {"PERF_FIXED_CTR_CTRL",     0x038D},
            {"PERF_GLOBAL_CTRL",        0x038F},
            {"PERF_GLOBAL_OVF_CTRL",    0x0390},
            {"PERF_FIXED_CTR0",         0x0309},
            {"PERF_FIXED_CTR1",         0x030A},
            {"PERF_FIXED_CTR2",         0x030B},
            {"C0_MSR_PMON_BOX_CTL",     0x0E00},
            {"C1_MSR_PMON_BOX_CTL",     0x0E10},
            {"C2_MSR_PMON_BOX_CTL",     0x0E20},
            {"C3_MSR_PMON_BOX_CTL",     0x0E30},
            {"C4_MSR_PMON_BOX_CTL",     0x0E40},
            {"C5_MSR_PMON_BOX_CTL",     0x0E50},
            {"C6_MSR_PMON_BOX_CTL",     0x0E60},
            {"C7_MSR_PMON_BOX_CTL",     0x0E70},
            {"C8_MSR_PMON_BOX_CTL",     0x0E80},
            {"C9_MSR_PMON_BOX_CTL",     0x0E90},
            {"C10_MSR_PMON_BOX_CTL",    0x0EA0},
            {"C11_MSR_PMON_BOX_CTL",    0x0EB0},
            {"C12_MSR_PMON_BOX_CTL",    0x0EC0},
            {"C13_MSR_PMON_BOX_CTL",    0x0ED0},
            {"C14_MSR_PMON_BOX_CTL",    0x0EE0},
            {"C15_MSR_PMON_BOX_CTL",    0x0EF0},
            {"C16_MSR_PMON_BOX_CTL",    0x0F00},
            {"C17_MSR_PMON_BOX_CTL",    0x0F10},
            {"C0_MSR_PMON_BOX_FILTER",  0x0E05},
            {"C1_MSR_PMON_BOX_FILTER",  0x0E15},
            {"C2_MSR_PMON_BOX_FILTER",  0x0E25},
            {"C3_MSR_PMON_BOX_FILTER",  0x0E35},
            {"C4_MSR_PMON_BOX_FILTER",  0x0E45},
            {"C5_MSR_PMON_BOX_FILTER",  0x0E55},
            {"C6_MSR_PMON_BOX_FILTER",  0x0E65},
            {"C7_MSR_PMON_BOX_FILTER",  0x0E75},
            {"C8_MSR_PMON_BOX_FILTER",  0x0E85},
            {"C9_MSR_PMON_BOX_FILTER",  0x0E95},
            {"C10_MSR_PMON_BOX_FILTER", 0x0EA5},
            {"C11_MSR_PMON_BOX_FILTER", 0x0EB5},
            {"C12_MSR_PMON_BOX_FILTER", 0x0EC5},
            {"C13_MSR_PMON_BOX_FILTER", 0x0ED5},
            {"C14_MSR_PMON_BOX_FILTER", 0x0EE5},
            {"C15_MSR_PMON_BOX_FILTER", 0x0EF5},
            {"C16_MSR_PMON_BOX_FILTER", 0x0F05},
            {"C17_MSR_PMON_BOX_FILTER", 0x0F15},
            {"C0_MSR_PMON_BOX_FILTER1", 0x0E06},
            {"C1_MSR_PMON_BOX_FILTER1", 0x0E16},
            {"C2_MSR_PMON_BOX_FILTER1", 0x0E26},
            {"C3_MSR_PMON_BOX_FILTER1", 0x0E36},
            {"C4_MSR_PMON_BOX_FILTER1", 0x0E46},
            {"C5_MSR_PMON_BOX_FILTER1", 0x0E56},
            {"C6_MSR_PMON_BOX_FILTER1", 0x0E66},
            {"C7_MSR_PMON_BOX_FILTER1", 0x0E76},
            {"C8_MSR_PMON_BOX_FILTER1", 0x0E86},
            {"C9_MSR_PMON_BOX_FILTER1", 0x0E96},
            {"C10_MSR_PMON_BOX_FILTER1",0x0EA6},
            {"C11_MSR_PMON_BOX_FILTER1",0x0EB6},
            {"C12_MSR_PMON_BOX_FILTER1",0x0EC6},
            {"C13_MSR_PMON_BOX_FILTER1",0x0ED6},
            {"C14_MSR_PMON_BOX_FILTER1",0x0EE6},
            {"C15_MSR_PMON_BOX_FILTER1",0x0EF6},
            {"C16_MSR_PMON_BOX_FILTER1",0x0F06},
            {"C17_MSR_PMON_BOX_FILTER1",0x0F16},
            {"C0_MSR_PMON_CTL0",        0x0E01},
            {"C1_MSR_PMON_CTL0",        0x0E11},
            {"C2_MSR_PMON_CTL0",        0x0E21},
            {"C3_MSR_PMON_CTL0",        0x0E31},
            {"C4_MSR_PMON_CTL0",        0x0E41},
            {"C5_MSR_PMON_CTL0",        0x0E51},
            {"C6_MSR_PMON_CTL0",        0x0E61},
            {"C7_MSR_PMON_CTL0",        0x0E71},
            {"C8_MSR_PMON_CTL0",        0x0E81},
            {"C9_MSR_PMON_CTL0",        0x0E91},
            {"C10_MSR_PMON_CTL0",       0x0EA1},
            {"C11_MSR_PMON_CTL0",       0x0EB1},
            {"C12_MSR_PMON_CTL0",       0x0EC1},
            {"C13_MSR_PMON_CTL0",       0x0ED1},
            {"C14_MSR_PMON_CTL0",       0x0EE1},
            {"C15_MSR_PMON_CTL0",       0x0EF1},
            {"C16_MSR_PMON_CTL0",       0x0F01},
            {"C17_MSR_PMON_CTL0",       0x0F11},
            {"C0_MSR_PMON_CTL1",        0x0E02},
            {"C1_MSR_PMON_CTL1",        0x0E12},
            {"C2_MSR_PMON_CTL1",        0x0E22},
            {"C3_MSR_PMON_CTL1",        0x0E32},
            {"C4_MSR_PMON_CTL1",        0x0E42},
            {"C5_MSR_PMON_CTL1",        0x0E52},
            {"C6_MSR_PMON_CTL1",        0x0E62},
            {"C7_MSR_PMON_CTL1",        0x0E72},
            {"C8_MSR_PMON_CTL1",        0x0E82},
            {"C9_MSR_PMON_CTL1",        0x0E92},
            {"C10_MSR_PMON_CTL1",       0x0EA2},
            {"C11_MSR_PMON_CTL1",       0x0EB2},
            {"C12_MSR_PMON_CTL1",       0x0EC2},
            {"C13_MSR_PMON_CTL1",       0x0ED2},
            {"C14_MSR_PMON_CTL1",       0x0EE2},
            {"C15_MSR_PMON_CTL1",       0x0EF2},
            {"C16_MSR_PMON_CTL1",       0x0F02},
            {"C17_MSR_PMON_CTL1",       0x0F12},
            {"C0_MSR_PMON_CTR0",        0x0E08},
            {"C1_MSR_PMON_CTR0",        0x0E18},
            {"C2_MSR_PMON_CTR0",        0x0E28},
            {"C3_MSR_PMON_CTR0",        0x0E38},
            {"C4_MSR_PMON_CTR0",        0x0E48},
            {"C5_MSR_PMON_CTR0",        0x0E58},
            {"C6_MSR_PMON_CTR0",        0x0E68},
            {"C7_MSR_PMON_CTR0",        0x0E78},
            {"C8_MSR_PMON_CTR0",        0x0E88},
            {"C9_MSR_PMON_CTR0",        0x0E98},
            {"C10_MSR_PMON_CTR0",       0x0EA8},
            {"C11_MSR_PMON_CTR0",       0x0EB8},
            {"C12_MSR_PMON_CTR0",       0x0EC8},
            {"C13_MSR_PMON_CTR0",       0x0ED8},
            {"C14_MSR_PMON_CTR0",       0x0EE8},
            {"C15_MSR_PMON_CTR0",       0x0EF8},
            {"C16_MSR_PMON_CTR0",       0x0F08},
            {"C17_MSR_PMON_CTR0",       0x0F18},
            {"C0_MSR_PMON_CTR1",        0x0E09},
            {"C1_MSR_PMON_CTR1",        0x0E19},
            {"C2_MSR_PMON_CTR1",        0x0E29},
            {"C3_MSR_PMON_CTR1",        0x0E39},
            {"C4_MSR_PMON_CTR1",        0x0E49},
            {"C5_MSR_PMON_CTR1",        0x0E59},
            {"C6_MSR_PMON_CTR1",        0x0E69},
            {"C7_MSR_PMON_CTR1",        0x0E79},
            {"C8_MSR_PMON_CTR1",        0x0E89},
            {"C9_MSR_PMON_CTR1",        0x0E99},
            {"C10_MSR_PMON_CTR1",       0x0EA9},
            {"C11_MSR_PMON_CTR1",       0x0EB9},
            {"C12_MSR_PMON_CTR1",       0x0EC9},
            {"C13_MSR_PMON_CTR1",       0x0ED9},
            {"C14_MSR_PMON_CTR1",       0x0EE9},
            {"C15_MSR_PMON_CTR1",       0x0EF9},
            {"C16_MSR_PMON_CTR1",       0x0F09},
            {"C17_MSR_PMON_CTR1",       0x0F19}
        };

        m_msr_offset_map = msr_map;

    }
}
