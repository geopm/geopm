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
#include "geopm_message.h"
#include "Exception.hpp"
#include "HSXPlatformImp.hpp"
#include "config.h"

namespace geopm
{
    HSXPlatformImp::HSXPlatformImp()
        : PlatformImp(3, 5, 8.0)
        , m_energy_units(1.0)
        , m_power_units(1.0)
        , m_min_pkg_watts(1)
        , m_max_pkg_watts(100)
        , m_min_pp0_watts(1)
        , m_max_pp0_watts(100)
        , m_min_dram_watts(1)
        , m_max_dram_watts(100)
        , m_platform_id(0x63F)
        , M_HSX_MODEL_NAME("Haswell E")
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

    }

    HSXPlatformImp::~HSXPlatformImp()
    {
        while(m_cpu_file_desc.size()) {
            close(m_cpu_file_desc.back());
            m_cpu_file_desc.pop_back();
        }
    }

    bool HSXPlatformImp::model_supported(int platform_id)
    {
        return (platform_id == m_platform_id);
    }

    std::string HSXPlatformImp::platform_name()
    {
        return M_HSX_MODEL_NAME;
    }

    int HSXPlatformImp::power_control_domain(void) const
    {
        return GEOPM_DOMAIN_PACKAGE;
    }

    int HSXPlatformImp::frequency_control_domain(void) const
    {
        return GEOPM_DOMAIN_CPU;
    }

    double HSXPlatformImp::read_signal(int device_type, int device_index, int signal_type)
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
                throw geopm::Exception("HSXPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }

        return value;
    }

    void HSXPlatformImp::write_control(int device_type, int device_index, int signal_type, double value)
    {
        uint64_t msr_val = 0;

        switch (signal_type) {
            case GEOPM_TELEMETRY_TYPE_PKG_ENERGY:
                if (value < m_min_pkg_watts) {
                    value = m_min_pkg_watts;
                }
                if (value > m_max_pkg_watts) {
                    value = m_max_pkg_watts;
                }
                msr_val = (uint64_t)(value * m_power_units);
                msr_val = msr_val | (msr_val << 32) | M_PKG_POWER_LIMIT_MASK_MAGIC;
                msr_write(device_type, device_index, m_control_msr_offset[0], msr_val);
                break;
            case GEOPM_TELEMETRY_TYPE_PP0_ENERGY:
                if (value < m_min_pp0_watts) {
                    value = m_min_pp0_watts;
                }
                if (value > m_max_pp0_watts) {
                    value = m_max_pp0_watts;
                }
                msr_val = (uint64_t)(value * m_power_units);
                msr_val = msr_val | (msr_val << 32) | M_PP0_POWER_LIMIT_MASK_MAGIC;
                msr_write(device_type, device_index, m_control_msr_offset[1], msr_val);
                break;
            case GEOPM_TELEMETRY_TYPE_DRAM_ENERGY:
                if (value < m_min_dram_watts) {
                    value = m_min_dram_watts;
                }
                if (value > m_max_dram_watts) {
                    value = m_max_dram_watts;
                }
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
                throw geopm::Exception("HSXPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
    }

    void HSXPlatformImp::msr_initialize()
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
        int cpu_per_socket = m_num_hw_cpu / m_num_package;
        for (int i = 0; i < m_num_hw_cpu; i++) {
            std::string msr_name("_MSR_PMON_CTR1");
            msr_name.insert(0, std::to_string(i % cpu_per_socket));
            msr_name.insert(0, "C");
            m_signal_msr_offset.push_back(msr_offset(msr_name));
        }

        //Save off the msr offsets for the controls we want to write to avoid a map lookup
        m_control_msr_offset.push_back(msr_offset("PKG_ENERGY_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("PKG_DRAM_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("PKG_PP0_LIMIT"));
        m_control_msr_offset.push_back(msr_offset("IA32_PERF_CTL"));
    }

    void HSXPlatformImp::msr_reset()
    {
        rapl_reset();
        cbo_counters_reset();
        fixed_counters_reset();
    }

    void HSXPlatformImp::rapl_init()
    {
        uint64_t tmp;

        //Make sure units are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT");
        m_energy_units = pow(0.5, (double)((tmp >> 8) & 0x1F));
        m_power_units = pow(2, (double)((tmp >> 0) & 0xF));

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "RAPL_POWER_UNIT");
            double energy = pow(0.5, (double)((tmp >> 8) & 0x1F));
            double power = pow(2, (double)((tmp >> 0) & 0xF));
            if (energy != m_energy_units || power != m_power_units) {
                throw Exception("detected inconsistent power units among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        //Make sure bounds are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_INFO");
        m_min_pkg_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units;
        m_max_pkg_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units;

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "DRAM_POWER_INFO");
        m_min_dram_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units;
        m_max_dram_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units;

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO");
            double pkg_min = ((double)((tmp >> 16) & 0x7fff)) / m_power_units;
            double pkg_max = ((double)((tmp >> 32) & 0x7fff)) / m_power_units;
            if (pkg_min != m_min_pkg_watts || pkg_max != m_max_pkg_watts) {
                throw Exception("detected inconsistent power pkg bounds among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
            double dram_min = ((double)((tmp >> 16) & 0x7fff)) / m_power_units;
            double dram_max = ((double)((tmp >> 32) & 0x7fff)) / m_power_units;
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
        int cpu_per_socket = m_num_hw_cpu / m_num_package;
        for (int i = 0; i < m_num_hw_cpu; i++) {
            std::string msr_name("_MSR_PMON_CTL1");
            std::string box_msr_name("_MSR_PMON_BOX_CTL");
            std::string filter_msr_name("_MSR_PMON_BOX_FILTER");
            box_msr_name.insert(0, std::to_string(i % cpu_per_socket));
            box_msr_name.insert(0, "C");
            msr_name.insert(0, std::to_string(i % cpu_per_socket));
            msr_name.insert(0, "C");
            filter_msr_name.insert(0, std::to_string(i % cpu_per_socket));
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

    void HSXPlatformImp::fixed_counters_init()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR_CTRL", 0x0333);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_CTRL", 0x70000000F);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void HSXPlatformImp::rapl_reset()
    {
        //clear power limits
        for (int i = 1; i < m_num_package; i++) {
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_LIMIT", 0x0);
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "PP0_POWER_LIMIT", 0x0);
            msr_write(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_LIMIT", 0x0);
        }

    }

    void HSXPlatformImp::cbo_counters_reset()
    {
        int cpu_per_socket = m_num_hw_cpu / m_num_package;
        for (int i = 0; i < m_num_hw_cpu; i++) {
            std::string msr_name("_MSR_PMON_BOX_CTL");
            msr_name.insert(0, std::to_string(i % cpu_per_socket));
            msr_name.insert(0, "C");
            // reset counters
            msr_write(GEOPM_DOMAIN_CPU, i, msr_name,
                      msr_read(GEOPM_DOMAIN_CPU, i, msr_name)
                      | M_RST_CTRS);
        }
    }

    void HSXPlatformImp::fixed_counters_reset()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR0", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR1", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR2", 0x0);
        }
    }

    void HSXPlatformImp::load_msr_offsets()
    {
        std::map<std::string,std::pair<off_t,unsigned long> > msr_map = {
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
            {"C0_MSR_PMON_BOX_CTL",     {0x0E00, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_CTL",     {0x0E10, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_CTL",     {0x0E20, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_CTL",     {0x0E30, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_CTL",     {0x0E40, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_CTL",     {0x0E50, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_CTL",     {0x0E60, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_CTL",     {0x0E70, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_CTL",     {0x0E80, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_CTL",     {0x0E90, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_CTL",    {0x0EA0, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_CTL",    {0x0EB0, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_CTL",    {0x0EC0, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_CTL",    {0x0ED0, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_CTL",    {0x0EE0, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_CTL",    {0x0EF0, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_CTL",    {0x0F00, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_CTL",    {0x0F10, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER",  {0x0E05, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER",  {0x0E15, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER",  {0x0E25, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER",  {0x0E35, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER",  {0x0E45, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER",  {0x0E55, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER",  {0x0E65, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER",  {0x0E75, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER",  {0x0E85, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER",  {0x0E95, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER", {0x0EA5, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER", {0x0EB5, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER", {0x0EC5, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER", {0x0ED5, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER", {0x0EE5, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_FILTER", {0x0EF5, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_FILTER", {0x0F05, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_FILTER", {0x0F15, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER1", {0x0E06, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER1", {0x0E16, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER1", {0x0E26, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER1", {0x0E36, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER1", {0x0E46, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER1", {0x0E56, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER1", {0x0E66, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER1", {0x0E76, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER1", {0x0E86, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER1", {0x0E96, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER1",{0x0EA6, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER1",{0x0EB6, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER1",{0x0EC6, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER1",{0x0ED6, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER1",{0x0EE6, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_FILTER1",{0x0EF6, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_FILTER1",{0x0F06, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_FILTER1",{0x0F16, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL0",        {0x0E01, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL0",        {0x0E11, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL0",        {0x0E21, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL0",        {0x0E31, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL0",        {0x0E41, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL0",        {0x0E51, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL0",        {0x0E61, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL0",        {0x0E71, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL0",        {0x0E81, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL0",        {0x0E91, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL0",       {0x0EA1, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL0",       {0x0EB1, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL0",       {0x0EC1, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL0",       {0x0ED1, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL0",       {0x0EE1, 0x00000000ffffffff}},
            {"C15_MSR_PMON_CTL0",       {0x0EF1, 0x00000000ffffffff}},
            {"C16_MSR_PMON_CTL0",       {0x0F01, 0x00000000ffffffff}},
            {"C17_MSR_PMON_CTL0",       {0x0F11, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL1",        {0x0E02, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL1",        {0x0E12, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL1",        {0x0E22, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL1",        {0x0E32, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL1",        {0x0E42, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL1",        {0x0E52, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL1",        {0x0E62, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL1",        {0x0E72, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL1",        {0x0E82, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL1",        {0x0E92, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL1",       {0x0EA2, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL1",       {0x0EB2, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL1",       {0x0EC2, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL1",       {0x0ED2, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL1",       {0x0EE2, 0x00000000ffffffff}},
            {"C15_MSR_PMON_CTL1",       {0x0EF2, 0x00000000ffffffff}},
            {"C16_MSR_PMON_CTL1",       {0x0F02, 0x00000000ffffffff}},
            {"C17_MSR_PMON_CTL1",       {0x0F12, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTR0",        {0x0E08, 0x0000000000000000}},
            {"C1_MSR_PMON_CTR0",        {0x0E18, 0x0000000000000000}},
            {"C2_MSR_PMON_CTR0",        {0x0E28, 0x0000000000000000}},
            {"C3_MSR_PMON_CTR0",        {0x0E38, 0x0000000000000000}},
            {"C4_MSR_PMON_CTR0",        {0x0E48, 0x0000000000000000}},
            {"C5_MSR_PMON_CTR0",        {0x0E58, 0x0000000000000000}},
            {"C6_MSR_PMON_CTR0",        {0x0E68, 0x0000000000000000}},
            {"C7_MSR_PMON_CTR0",        {0x0E78, 0x0000000000000000}},
            {"C8_MSR_PMON_CTR0",        {0x0E88, 0x0000000000000000}},
            {"C9_MSR_PMON_CTR0",        {0x0E98, 0x0000000000000000}},
            {"C10_MSR_PMON_CTR0",       {0x0EA8, 0x0000000000000000}},
            {"C11_MSR_PMON_CTR0",       {0x0EB8, 0x0000000000000000}},
            {"C12_MSR_PMON_CTR0",       {0x0EC8, 0x0000000000000000}},
            {"C13_MSR_PMON_CTR0",       {0x0ED8, 0x0000000000000000}},
            {"C14_MSR_PMON_CTR0",       {0x0EE8, 0x0000000000000000}},
            {"C15_MSR_PMON_CTR0",       {0x0EF8, 0x0000000000000000}},
            {"C16_MSR_PMON_CTR0",       {0x0F08, 0x0000000000000000}},
            {"C17_MSR_PMON_CTR0",       {0x0F18, 0x0000000000000000}},
            {"C0_MSR_PMON_CTR1",        {0x0E09, 0x0000000000000000}},
            {"C1_MSR_PMON_CTR1",        {0x0E19, 0x0000000000000000}},
            {"C2_MSR_PMON_CTR1",        {0x0E29, 0x0000000000000000}},
            {"C3_MSR_PMON_CTR1",        {0x0E39, 0x0000000000000000}},
            {"C4_MSR_PMON_CTR1",        {0x0E49, 0x0000000000000000}},
            {"C5_MSR_PMON_CTR1",        {0x0E59, 0x0000000000000000}},
            {"C6_MSR_PMON_CTR1",        {0x0E69, 0x0000000000000000}},
            {"C7_MSR_PMON_CTR1",        {0x0E79, 0x0000000000000000}},
            {"C8_MSR_PMON_CTR1",        {0x0E89, 0x0000000000000000}},
            {"C9_MSR_PMON_CTR1",        {0x0E99, 0x0000000000000000}},
            {"C10_MSR_PMON_CTR1",       {0x0EA9, 0x0000000000000000}},
            {"C11_MSR_PMON_CTR1",       {0x0EB9, 0x0000000000000000}},
            {"C12_MSR_PMON_CTR1",       {0x0EC9, 0x0000000000000000}},
            {"C13_MSR_PMON_CTR1",       {0x0ED9, 0x0000000000000000}},
            {"C14_MSR_PMON_CTR1",       {0x0EE9, 0x0000000000000000}},
            {"C15_MSR_PMON_CTR1",       {0x0EF9, 0x0000000000000000}},
            {"C16_MSR_PMON_CTR1",       {0x0F09, 0x0000000000000000}},
            {"C17_MSR_PMON_CTR1",       {0x0F19, 0x0000000000000000}}
        };

        m_msr_offset_map = msr_map;

    }
}
