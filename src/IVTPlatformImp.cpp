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

#include <math.h>
#include <unistd.h>
#include "geopm_error.h"
#include "geopm_message.h"
#include "Exception.hpp"
#include "IVTPlatformImp.hpp"

namespace geopm
{
    IVTPlatformImp::IVTPlatformImp()
        : PlatformImp(3, 5)
        , m_energy_units(1.0)
        , m_power_units(1.0)
        , m_min_pkg_watts(1)
        , m_max_pkg_watts(100)
        , m_min_pp0_watts(1)
        , m_max_pp0_watts(100)
        , m_min_dram_watts(1)
        , m_max_dram_watts(100)
        , m_platform_id(0)
        , M_SNB_PLATFORM_ID(0x62D)
        , M_IVT_PLATFORM_ID(0x63E)
        , M_SNB_MODEL_NAME("Sandybridge E")
        , M_IVT_MODEL_NAME("Ivybridge E")
        , M_BOX_FRZ_EN(0x1 << 16)
        , M_BOX_FRZ(0x1 << 8)
        , M_CTR_EN(0x1 << 22)
        , M_RST_CTRS(0x1 << 1)
        , M_LLC_FILTER_MASK(0x1F << 18)
        , M_LLC_VICTIMS_EV_SEL(0x37)
        , M_LLC_VICTIMS_UMASK(0x7 << 8)
        , M_EVENT_SEL_0(M_LLC_VICTIMS_EV_SEL)
        , M_UMASK_0(M_LLC_VICTIMS_UMASK)
        , M_PKG_POWER_LIMIT_MASK_MAGIC(0x0007800000078000ul)
        , M_DRAM_POWER_LIMIT_MASK_MAGIC(0xfefffful & M_PKG_POWER_LIMIT_MASK_MAGIC)
        , M_PP0_POWER_LIMIT_MASK_MAGIC(0xfffffful & M_PKG_POWER_LIMIT_MASK_MAGIC)
    {
        m_num_cpu_signal = 5;
        m_num_package_signal = 3;
    }

    IVTPlatformImp::~IVTPlatformImp()
    {
        while(m_cpu_file_desc.size()) {
            close(m_cpu_file_desc.back());
            m_cpu_file_desc.pop_back();
        }
    }

    bool IVTPlatformImp::model_supported(int platform_id)
    {
        m_platform_id = platform_id;
        return (M_IVT_PLATFORM_ID == platform_id || M_SNB_PLATFORM_ID == platform_id);
    }

    std::string IVTPlatformImp::platform_name()
    {
        return M_IVT_MODEL_NAME;
    }

    int IVTPlatformImp::power_control_domain(void) const
    {
        return GEOPM_DOMAIN_PACKAGE;
    }

    int IVTPlatformImp::frequency_control_domain(void) const
    {
        return GEOPM_DOMAIN_CPU;
    }

    double IVTPlatformImp::read_signal(int device_type, int device_index, int signal_type)
    {
        double value = 0.0;
        int offset_idx = 0;

        switch (signal_type) {
            case GEOPM_TELEMETRY_TYPE_PKG_ENERGY:
                offset_idx = device_index * m_num_package_signal;
                value = msr_overflow(offset_idx, 32,
                                     (double)msr_read(device_type, device_index,
                                                      m_signal_msr_offset[0]));
                value *= m_energy_units;
                break;
            case GEOPM_TELEMETRY_TYPE_PP0_ENERGY:
                offset_idx = device_index * m_num_package_signal + 1;
                value = msr_overflow(offset_idx, 32,
                                     (double)msr_read(device_type, device_index,
                                                      m_signal_msr_offset[1]));
                value *= m_energy_units;
                break;
            case GEOPM_TELEMETRY_TYPE_DRAM_ENERGY:
                offset_idx = device_index * m_num_package_signal + 2;
                value = msr_overflow(offset_idx, 32,
                                     (double)msr_read(device_type, device_index,
                                                      m_signal_msr_offset[2]));
                value *= m_energy_units;
                break;
            case GEOPM_TELEMETRY_TYPE_FREQUENCY:
                offset_idx = m_num_package * m_num_package_signal + device_index * m_num_cpu_signal;
                value = (double)(msr_read(device_type, device_index / m_num_cpu_per_core,
                                          m_signal_msr_offset[3]) >> 8);
                //convert to MHZ
                value *= 0.1;
                break;
            case GEOPM_TELEMETRY_TYPE_INST_RETIRED:
                offset_idx = m_num_package * m_num_package_signal + device_index * m_num_cpu_signal + 1;
                value = msr_overflow(offset_idx, 64,
                                     (double)msr_read(device_type, device_index / m_num_cpu_per_core,
                                                      m_signal_msr_offset[4]));
                break;
            case GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE:
                offset_idx = m_num_package * m_num_package_signal + device_index * m_num_cpu_signal + 2;
                value = msr_overflow(offset_idx, 64,
                                     (double)msr_read(device_type, device_index / m_num_cpu_per_core,
                                                      m_signal_msr_offset[5]));
                break;
            case GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF:
                offset_idx = m_num_package * m_num_package_signal + device_index * m_num_cpu_signal + 3;
                value = msr_overflow(offset_idx, 64,
                                     (double)msr_read(device_type, device_index / m_num_cpu_per_core,
                                                      m_signal_msr_offset[6]));
                break;
            case GEOPM_TELEMETRY_TYPE_LLC_VICTIMS:
                offset_idx = m_num_package * m_num_package_signal + device_index * m_num_cpu_signal + 4;
                value = msr_overflow(offset_idx, 44,
                                     (double)msr_read(device_type, device_index / m_num_cpu_per_core,
                                                      m_signal_msr_offset[7 + device_index]));
                break;
            default:
                throw geopm::Exception("IVTPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }

        return value;
    }

    void IVTPlatformImp::write_control(int device_type, int device_index, int signal_type, double value)
    {
        uint64_t msr_val = 0;

        switch (signal_type) {
            case GEOPM_TELEMETRY_TYPE_PKG_ENERGY:
                msr_val = (uint64_t)(value * m_power_units);
                msr_val = msr_val | (msr_val << 32) | M_PKG_POWER_LIMIT_MASK_MAGIC;
                msr_write(device_type, device_index, m_control_msr_offset[0], msr_val);
                break;
            case GEOPM_TELEMETRY_TYPE_PP0_ENERGY:
                msr_val = (uint64_t)(value * m_power_units);
                msr_val = msr_val | (msr_val << 32) | M_PP0_POWER_LIMIT_MASK_MAGIC;
                msr_write(device_type, device_index, m_control_msr_offset[1], msr_val);
                break;
            case GEOPM_TELEMETRY_TYPE_DRAM_ENERGY:
                msr_val = (uint64_t)(value * m_power_units);
                msr_val = msr_val | (msr_val << 32) | M_DRAM_POWER_LIMIT_MASK_MAGIC;
                msr_write(device_type, device_index, m_control_msr_offset[2], msr_val);
                break;
            case GEOPM_TELEMETRY_TYPE_FREQUENCY:
                msr_val = (uint64_t)(value * 10);
                msr_val = msr_val << 8;
                msr_write(device_type, device_index / m_num_cpu_per_core, m_control_msr_offset[3], msr_val);
                break;
            default:
                throw geopm::Exception("IVTPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
    }

    void IVTPlatformImp::msr_initialize()
    {
        for (int i = 0; i < m_num_logical_cpu; i++) {
            msr_open(i);
        }
        load_msr_offsets();
        rapl_init();
        cbo_counters_init();
        fixed_counters_init();

        //Save off the msr offsets for the signals we want to read to avoid a map lookup
        m_signal_msr_offset.push_back(msr_offset("PKG_ENERGY_STATUS"));
        m_signal_msr_offset.push_back(msr_offset("PP0_ENERGY_STATUS"));
        m_signal_msr_offset.push_back(msr_offset("DRAM_ENERGY_STATUS"));
        m_signal_msr_offset.push_back(msr_offset("IA32_PERF_STATUS"));
        m_signal_msr_offset.push_back(msr_offset("PERF_FIXED_CTR0"));
        m_signal_msr_offset.push_back(msr_offset("PERF_FIXED_CTR1"));
        m_signal_msr_offset.push_back(msr_offset("PERF_FIXED_CTR2"));
        for (int i = 0; i < m_num_hw_cpu; i++) {
            std::string msr_name("_MSR_PMON_CTR1");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            m_signal_msr_offset.push_back(msr_offset(msr_name));
        }

        //Save off the msr offsets for the controls we want to write to avoid a map lookup
        m_control_msr_offset.push_back(msr_offset("PKG_ENERGY_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("PKG_DRAM_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("PKG_PP0_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("IA32_PERF_CTL"));
    }

    void IVTPlatformImp::msr_reset()
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
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT");
        energy_units = pow(0.5, (double)((tmp >> 8) & 0x1F));
        power_units = pow(2, (double)((tmp >> 0) & 0xF));

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "RAPL_POWER_UNIT");
            double energy = pow(0.5, (double)((tmp >> 8) & 0x1F));
            double power = pow(2, (double)((tmp >> 0) & 0xF));
            if (energy != energy_units || power != power_units) {
                throw Exception("detected inconsistent power units among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        //Make sure bounds are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_INFO");
        m_min_pkg_watts = ((double)((tmp >> 16) & 0x7fff)) / power_units;
        m_max_pkg_watts = ((double)((tmp >> 32) & 0x7fff)) / power_units;

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "DRAM_POWER_INFO");
        m_min_dram_watts = ((double)((tmp >> 16) & 0x7fff)) / power_units;
        m_max_dram_watts = ((double)((tmp >> 32) & 0x7fff)) / power_units;

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO");
            double pkg_min = ((double)((tmp >> 16) & 0x7fff)) / power_units;
            double pkg_max = ((double)((tmp >> 32) & 0x7fff)) / power_units;
            if (pkg_min != m_min_pkg_watts || pkg_max != m_max_pkg_watts) {
                throw Exception("detected inconsistent power pkg bounds among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
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

    void IVTPlatformImp::cbo_counters_init()
    {
        for (int i = 0; i < m_num_hw_cpu; i++) {
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
            msr_write(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | M_BOX_FRZ_EN);
            // freeze box
            msr_write(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | M_BOX_FRZ);
            msr_write(GEOPM_DOMAIN_CPU, i, msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, msr_name)
                      | M_CTR_EN);
            msr_write(GEOPM_DOMAIN_CPU, i, filter_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, filter_msr_name)
                      | M_LLC_FILTER_MASK);
            // llc victims
            msr_write(GEOPM_DOMAIN_CPU, i, msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, msr_name)
                      | M_EVENT_SEL_0 | M_UMASK_0);
            // reset counters
            msr_write(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      | M_RST_CTRS);
            /// @bug is this needed???
            msr_write(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      & ~M_BOX_FRZ);
            // unfreeze box
            msr_write(GEOPM_DOMAIN_CPU, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, box_msr_name)
                      & ~M_BOX_FRZ);
        }
    }

    void IVTPlatformImp::fixed_counters_init()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR_CTRL", 0x0333);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_CTRL", 0x70000000F);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void IVTPlatformImp::rapl_reset()
    {
        //clear power limits
        for (int i = 1; i < m_num_package; i++) {
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", 0x0);
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT", 0x0);
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT", 0x0);
        }

    }

    void IVTPlatformImp::cbo_counters_reset()
    {
        for (int i = 0; i < m_num_hw_cpu; i++) {
            std::string msr_name("_MSR_PMON_BOX_CTL");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            // reset counters
            msr_write(GEOPM_DOMAIN_CPU, i, msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, msr_name)
                      | M_RST_CTRS);
        }
    }

    void IVTPlatformImp::fixed_counters_reset()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR0", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR1", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR2", 0x0);
        }
    }

    void IVTPlatformImp::load_msr_offsets()
    {
        std::map<std::string,std::pair<off_t, unsigned long> > msr_map = {
            {"IA32_PERF_STATUS",        {0x0198, 0x0000000000000000}},
            {"IA32_PERF_CTL",           {0x0199, 0x000000010000ffff}},
            {"RAPL_POWER_UNIT",         {0x0606, 0x0000000000000000}},
            {"PKG_POWER_LIMIT",         {0x0610, 0x00ffffff00ffffff}},
            {"PKG_ENERGY_STATUS",       {0x0611, 0x0000000000000000}},
            {"PKG_POWER_INFO",          {0x0614, 0x0000000000000000}},
            {"PP0_POWER_LIMIT",         {0x0638, 0x0000000000ffffff}},
            {"PP0_ENERGY_STATUS",       {0x0639, 0x0000000000000000}},
            {"DRAM_POWER_LIMIT",        {0x0618, 0x0000000000ffffff}},
            {"DRAM_ENERGY_STATUS",      {0x0619, 0x0000000000000000}},
            {"DRAM_PERF_STATUS",        {0x061B, 0x0000000000000000}},
            {"DRAM_POWER_INFO",         {0x061C, 0x0000000000000000}},
            {"PERF_FIXED_CTR_CTRL",     {0x038D, 0x0000000000000bbb}},
            {"PERF_GLOBAL_CTRL",        {0x038F, 0x0000000700000003}},
            {"PERF_GLOBAL_OVF_CTRL",    {0x0390, 0xc000000700000003}},
            {"PERF_FIXED_CTR0",         {0x0309, 0xffffffffffffffff}},
            {"PERF_FIXED_CTR1",         {0x030A, 0xffffffffffffffff}},
            {"PERF_FIXED_CTR2",         {0x030B, 0xffffffffffffffff}},
            {"C0_MSR_PMON_BOX_CTL",     {0x0D04, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_CTL",     {0x0D24, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_CTL",     {0x0D44, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_CTL",     {0x0D64, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_CTL",     {0x0D84, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_CTL",     {0x0DA4, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_CTL",     {0x0DC4, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_CTL",     {0x0DE4, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_CTL",     {0x0E04, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_CTL",     {0x0E24, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_CTL",    {0x0E44, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_CTL",    {0x0E64, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_CTL",    {0x0E84, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_CTL",    {0x0EA4, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_CTL",    {0x0EC4, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER",  {0x0D14, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER",  {0x0D34, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER",  {0x0D54, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER",  {0x0D74, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER",  {0x0D94, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER",  {0x0DB4, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER",  {0x0DD4, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER",  {0x0DF4, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER",  {0x0E14, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER",  {0x0E34, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER", {0x0E54, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER", {0x0E74, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER", {0x0E94, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER", {0x0EB4, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER", {0x0ED4, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER1", {0x0D1A, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER1", {0x0D3A, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER1", {0x0D5A, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER1", {0x0D7A, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER1", {0x0D9A, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER1", {0x0DBA, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER1", {0x0DDA, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER1", {0x0DFA, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER1", {0x0E1A, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER1", {0x0E3A, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER1",{0x0E5A, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER1",{0x0E7A, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER1",{0x0E9A, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER1",{0x0EBA, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER1",{0x0EDA, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL0",        {0x0D10, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL0",        {0x0D30, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL0",        {0x0D50, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL0",        {0x0D70, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL0",        {0x0D90, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL0",        {0x0DB0, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL0",        {0x0DD0, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL0",        {0x0DF0, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL0",        {0x0E10, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL0",        {0x0E30, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL0",       {0x0E50, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL0",       {0x0E70, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL0",       {0x0E90, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL0",       {0x0EB0, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL0",       {0x0ED0, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL1",        {0x0D11, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL1",        {0x0D31, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL1",        {0x0D51, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL1",        {0x0D71, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL1",        {0x0D91, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL1",        {0x0DB1, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL1",        {0x0DD1, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL1",        {0x0DF1, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL1",        {0x0E11, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL1",        {0x0E31, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL1",       {0x0E51, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL1",       {0x0E71, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL1",       {0x0E91, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL1",       {0x0EB1, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL1",       {0x0ED1, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTR0",        {0x0D16, 0x0000000000000000}},
            {"C1_MSR_PMON_CTR0",        {0x0D36, 0x0000000000000000}},
            {"C2_MSR_PMON_CTR0",        {0x0D56, 0x0000000000000000}},
            {"C3_MSR_PMON_CTR0",        {0x0D76, 0x0000000000000000}},
            {"C4_MSR_PMON_CTR0",        {0x0D96, 0x0000000000000000}},
            {"C5_MSR_PMON_CTR0",        {0x0DB6, 0x0000000000000000}},
            {"C6_MSR_PMON_CTR0",        {0x0DD6, 0x0000000000000000}},
            {"C7_MSR_PMON_CTR0",        {0x0DF6, 0x0000000000000000}},
            {"C8_MSR_PMON_CTR0",        {0x0E16, 0x0000000000000000}},
            {"C9_MSR_PMON_CTR0",        {0x0E36, 0x0000000000000000}},
            {"C10_MSR_PMON_CTR0",       {0x0E56, 0x0000000000000000}},
            {"C11_MSR_PMON_CTR0",       {0x0E76, 0x0000000000000000}},
            {"C12_MSR_PMON_CTR0",       {0x0E96, 0x0000000000000000}},
            {"C13_MSR_PMON_CTR0",       {0x0EB6, 0x0000000000000000}},
            {"C14_MSR_PMON_CTR0",       {0x0ED6, 0x0000000000000000}},
            {"C0_MSR_PMON_CTR1",        {0x0D17, 0x0000000000000000}},
            {"C1_MSR_PMON_CTR1",        {0x0D37, 0x0000000000000000}},
            {"C2_MSR_PMON_CTR1",        {0x0D57, 0x0000000000000000}},
            {"C3_MSR_PMON_CTR1",        {0x0D77, 0x0000000000000000}},
            {"C4_MSR_PMON_CTR1",        {0x0D97, 0x0000000000000000}},
            {"C5_MSR_PMON_CTR1",        {0x0DB7, 0x0000000000000000}},
            {"C6_MSR_PMON_CTR1",        {0x0DD7, 0x0000000000000000}},
            {"C7_MSR_PMON_CTR1",        {0x0DF7, 0x0000000000000000}},
            {"C8_MSR_PMON_CTR1",        {0x0E17, 0x0000000000000000}},
            {"C9_MSR_PMON_CTR1",        {0x0E37, 0x0000000000000000}},
            {"C10_MSR_PMON_CTR1",       {0x0E57, 0x0000000000000000}},
            {"C11_MSR_PMON_CTR1",       {0x0E77, 0x0000000000000000}},
            {"C12_MSR_PMON_CTR1",       {0x0E97, 0x0000000000000000}},
            {"C13_MSR_PMON_CTR1",       {0x0EB7, 0x0000000000000000}},
            {"C14_MSR_PMON_CTR1",       {0x0ED7, 0x0000000000000000}}
        };
        m_msr_offset_map = msr_map;
    }
}
