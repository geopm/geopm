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
#include "IVTPlatformImp.hpp"

namespace geopm
{
    static const int ivb_id = 0x63E;
    static const int snb_id = 0x62D;
    static const std::string ivb_model_name = "Ivybridge E";
    static const std::string snb_model_name = "Sandybridge E";

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

    IVTPlatformImp::IVTPlatformImp()
        : m_energy_units(1.0)
        , m_power_units(1.0)
        , m_min_pkg_watts(1)
        , m_max_pkg_watts(100)
        , m_min_pp0_watts(1)
        , m_max_pp0_watts(100)
        , m_min_dram_watts(1)
        , m_max_dram_watts(100)
        , m_platform_id(ivb_id)
    {
        parse_hw_topology();
        initialize_msrs();
    }

    IVTPlatformImp::~IVTPlatformImp()
    {
        while(m_cpu_file_descs.size()) {
            close(m_cpu_file_descs.back());
            m_cpu_file_descs.pop_back();
        }
    }

    bool IVTPlatformImp::model_supported(int platform_id)
    {
        m_platform_id = platform_id;
        return (platform_id == ivb_id || platform_id == snb_id);
    }

    std::string IVTPlatformImp::get_platform_name()
    {
        if (m_platform_id == ivb_id) {
            return ivb_model_name;
        }
        else {
            return snb_model_name;
        }
    }

    void IVTPlatformImp::initialize_msrs()
    {
        for (int i = 0; i < m_num_cpu; i++) {
            open_msr(i);
        }
        load_msr_offsets();
        rapl_init();
        cbo_counters_init();
        fixed_counters_init();
    }

    void IVTPlatformImp::reset_msrs()
    {
        rapl_reset();
        cbo_counters_reset();
        fixed_counters_reset();
    }

    void IVTPlatformImp::rapl_init()
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
                throw std::runtime_error("detected inconsistent power units among packages");
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
                throw std::runtime_error("detected inconsistent power pkg bounds among packages");
            }
            tmp = read_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
            double dram_min = ((double)((tmp >> 16) & 0x7fff)) / power_units;
            double dram_max = ((double)((tmp >> 32) & 0x7fff)) / power_units;
            if (dram_min != m_min_dram_watts || dram_max != m_max_dram_watts) {
                throw std::runtime_error("detected inconsistent power dram bounds among packages");
            }
        }
        m_min_pp0_watts = m_min_pkg_watts;
        m_max_pp0_watts = m_max_pkg_watts;

        rapl_reset();
    }

    void IVTPlatformImp::cbo_counters_init()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = m_hyperthreaded ? msr_num/2 : msr_num;
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

    void IVTPlatformImp::fixed_counters_init()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = m_hyperthreaded ? msr_num/2 : msr_num;
        for (int cpu = 0; cpu < msr_num; cpu++) {
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR_CTRL", 0x0333);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_CTRL", 0x70000000F);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void IVTPlatformImp::rapl_reset()
    {
        //clear power limits
        for (int i = 1; i < m_num_package; i++) {
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", 0x0);
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT", 0x0);
            write_msr(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT", 0x0);
        }

    }

    void IVTPlatformImp::cbo_counters_reset()
    {
        // FIXME: Instead of resetting, should we restore state?
        int msr_num = m_num_cpu/m_num_package;
        msr_num = m_hyperthreaded ? msr_num/2 : msr_num;
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

    void IVTPlatformImp::fixed_counters_reset()
    {
        int msr_num = m_num_cpu/m_num_package;
        msr_num = m_hyperthreaded ? msr_num/2 : msr_num;
        for (int cpu = 0; cpu < msr_num; cpu++) {
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR0", 0x0);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR1", 0x0);
            write_msr(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR2", 0x0);
        }
    }

    void IVTPlatformImp::load_msr_offsets()
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
            {"C0_MSR_PMON_BOX_CTL",     0x0D04},
            {"C1_MSR_PMON_BOX_CTL",     0x0D24},
            {"C2_MSR_PMON_BOX_CTL",     0x0D44},
            {"C3_MSR_PMON_BOX_CTL",     0x0D64},
            {"C4_MSR_PMON_BOX_CTL",     0x0D84},
            {"C5_MSR_PMON_BOX_CTL",     0x0DA4},
            {"C6_MSR_PMON_BOX_CTL",     0x0DC4},
            {"C7_MSR_PMON_BOX_CTL",     0x0DE4},
            {"C8_MSR_PMON_BOX_CTL",     0x0E04},
            {"C9_MSR_PMON_BOX_CTL",     0x0E24},
            {"C10_MSR_PMON_BOX_CTL",    0x0E44},
            {"C11_MSR_PMON_BOX_CTL",    0x0E64},
            {"C12_MSR_PMON_BOX_CTL",    0x0E84},
            {"C13_MSR_PMON_BOX_CTL",    0x0EA4},
            {"C14_MSR_PMON_BOX_CTL",    0x0EC4},
            {"C0_MSR_PMON_BOX_FILTER",  0x0D14},
            {"C1_MSR_PMON_BOX_FILTER",  0x0D34},
            {"C2_MSR_PMON_BOX_FILTER",  0x0D54},
            {"C3_MSR_PMON_BOX_FILTER",  0x0D74},
            {"C4_MSR_PMON_BOX_FILTER",  0x0D94},
            {"C5_MSR_PMON_BOX_FILTER",  0x0DB4},
            {"C6_MSR_PMON_BOX_FILTER",  0x0DD4},
            {"C7_MSR_PMON_BOX_FILTER",  0x0DF4},
            {"C8_MSR_PMON_BOX_FILTER",  0x0E14},
            {"C9_MSR_PMON_BOX_FILTER",  0x0E34},
            {"C10_MSR_PMON_BOX_FILTER", 0x0E54},
            {"C11_MSR_PMON_BOX_FILTER", 0x0E74},
            {"C12_MSR_PMON_BOX_FILTER", 0x0E94},
            {"C13_MSR_PMON_BOX_FILTER", 0x0EB4},
            {"C14_MSR_PMON_BOX_FILTER", 0x0ED4},
            {"C0_MSR_PMON_BOX_FILTER1", 0x0D1A},
            {"C1_MSR_PMON_BOX_FILTER1", 0x0D3A},
            {"C2_MSR_PMON_BOX_FILTER1", 0x0D5A},
            {"C3_MSR_PMON_BOX_FILTER1", 0x0D7A},
            {"C4_MSR_PMON_BOX_FILTER1", 0x0D9A},
            {"C5_MSR_PMON_BOX_FILTER1", 0x0DBA},
            {"C6_MSR_PMON_BOX_FILTER1", 0x0DDA},
            {"C7_MSR_PMON_BOX_FILTER1", 0x0DFA},
            {"C8_MSR_PMON_BOX_FILTER1", 0x0E1A},
            {"C9_MSR_PMON_BOX_FILTER1", 0x0E3A},
            {"C10_MSR_PMON_BOX_FILTER1",0x0E5A},
            {"C11_MSR_PMON_BOX_FILTER1",0x0E7A},
            {"C12_MSR_PMON_BOX_FILTER1",0x0E9A},
            {"C13_MSR_PMON_BOX_FILTER1",0x0EBA},
            {"C14_MSR_PMON_BOX_FILTER1",0x0EDA},
            {"C0_MSR_PMON_CTL0",        0x0D10},
            {"C1_MSR_PMON_CTL0",        0x0D30},
            {"C2_MSR_PMON_CTL0",        0x0D50},
            {"C3_MSR_PMON_CTL0",        0x0D70},
            {"C4_MSR_PMON_CTL0",        0x0D90},
            {"C5_MSR_PMON_CTL0",        0x0DB0},
            {"C6_MSR_PMON_CTL0",        0x0DD0},
            {"C7_MSR_PMON_CTL0",        0x0DF0},
            {"C8_MSR_PMON_CTL0",        0x0E10},
            {"C9_MSR_PMON_CTL0",        0x0E30},
            {"C10_MSR_PMON_CTL0",       0x0E50},
            {"C11_MSR_PMON_CTL0",       0x0E70},
            {"C12_MSR_PMON_CTL0",       0x0E90},
            {"C13_MSR_PMON_CTL0",       0x0EB0},
            {"C14_MSR_PMON_CTL0",       0x0ED0},
            {"C0_MSR_PMON_CTL1",        0x0D11},
            {"C1_MSR_PMON_CTL1",        0x0D31},
            {"C2_MSR_PMON_CTL1",        0x0D51},
            {"C3_MSR_PMON_CTL1",        0x0D71},
            {"C4_MSR_PMON_CTL1",        0x0D91},
            {"C5_MSR_PMON_CTL1",        0x0DB1},
            {"C6_MSR_PMON_CTL1",        0x0DD1},
            {"C7_MSR_PMON_CTL1",        0x0DF1},
            {"C8_MSR_PMON_CTL1",        0x0E11},
            {"C9_MSR_PMON_CTL1",        0x0E31},
            {"C10_MSR_PMON_CTL1",       0x0E51},
            {"C11_MSR_PMON_CTL1",       0x0E71},
            {"C12_MSR_PMON_CTL1",       0x0E91},
            {"C13_MSR_PMON_CTL1",       0x0EB1},
            {"C14_MSR_PMON_CTL1",       0x0ED1},
            {"C0_MSR_PMON_CTR0",        0x0D16},
            {"C1_MSR_PMON_CTR0",        0x0D36},
            {"C2_MSR_PMON_CTR0",        0x0D56},
            {"C3_MSR_PMON_CTR0",        0x0D76},
            {"C4_MSR_PMON_CTR0",        0x0D96},
            {"C5_MSR_PMON_CTR0",        0x0DB6},
            {"C6_MSR_PMON_CTR0",        0x0DD6},
            {"C7_MSR_PMON_CTR0",        0x0DF6},
            {"C8_MSR_PMON_CTR0",        0x0E16},
            {"C9_MSR_PMON_CTR0",        0x0E36},
            {"C10_MSR_PMON_CTR0",       0x0E56},
            {"C11_MSR_PMON_CTR0",       0x0E76},
            {"C12_MSR_PMON_CTR0",       0x0E96},
            {"C13_MSR_PMON_CTR0",       0x0EB6},
            {"C14_MSR_PMON_CTR0",       0x0ED6},
            {"C0_MSR_PMON_CTR1",        0x0D17},
            {"C1_MSR_PMON_CTR1",        0x0D37},
            {"C2_MSR_PMON_CTR1",        0x0D57},
            {"C3_MSR_PMON_CTR1",        0x0D77},
            {"C4_MSR_PMON_CTR1",        0x0D97},
            {"C5_MSR_PMON_CTR1",        0x0DB7},
            {"C6_MSR_PMON_CTR1",        0x0DD7},
            {"C7_MSR_PMON_CTR1",        0x0DF7},
            {"C8_MSR_PMON_CTR1",        0x0E17},
            {"C9_MSR_PMON_CTR1",        0x0E37},
            {"C10_MSR_PMON_CTR1",       0x0E57},
            {"C11_MSR_PMON_CTR1",       0x0E77},
            {"C12_MSR_PMON_CTR1",       0x0E97},
            {"C13_MSR_PMON_CTR1",       0x0EB7},
            {"C14_MSR_PMON_CTR1",       0x0ED7}
        };

        m_msr_offset_map = msr_map;

    }
}
