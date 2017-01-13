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

#include <cmath>
#include <sstream>
#include <unistd.h>
#include "geopm_error.h"
#include "geopm_message.h"
#include "Exception.hpp"
#include "KNLPlatformImp.hpp"

namespace geopm
{
    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &knl_msr_signal_map(void);
    static const std::map<std::string, std::pair<off_t, uint64_t> > &knl_msr_control_map(void);
    static const std::map<std::string, std::string> signal_to_msr_map {
        {"pkg_energy", "PKG_ENERGY_STATUS"},
        {"dram_energy", "DRAM_ENERGY_STATUS"},
        {"frequency", "IA32_PERF_STATUS"},
        {"instructions_retired", "PERF_FIXED_CTR0"},
        {"clock_unhalted_core", "PERF_FIXED_CTR1"},
        {"clock_unhalted_ref", "PERF_FIXED_CTR2"},
        {"l2_cache_victims", "event-0x4123"},
        {"l2_prefetches", "event-0x43e"}
    };

    int KNLPlatformImp::platform_id(void)
    {
        return 0x657;
    }

    KNLPlatformImp::KNLPlatformImp()
        : PlatformImp({{GEOPM_DOMAIN_CONTROL_POWER, 50.0},{GEOPM_DOMAIN_CONTROL_FREQUENCY, 1.0}},
          &(knl_msr_signal_map()), 
          &(knl_msr_control_map()))
        , m_throttle_limit_mhz(0.5)
        , m_energy_units(1.0)
        , m_power_units_inv(1.0)
        , m_dram_energy_units(1.5258789063E-5)
        , m_min_pkg_watts(1.0)
        , m_max_pkg_watts(100.0)
        , m_min_dram_watts(1.0)
        , m_max_dram_watts(100)
        , m_min_freq_mhz(1000.0)
        , m_max_freq_mhz(1200.0)
        , m_freq_step_mhz(100.0)
        , m_control_msr_pair(M_NUM_CONTROL)
        , m_pkg_power_limit_static(0)
        , M_BOX_FRZ_EN(0x1 << 16)
        , M_BOX_FRZ(0x1 << 8)
        , M_CTR_EN(0x1 << 22)
        , M_RST_CTRS(0x1 << 1)
        , M_DRAM_POWER_LIMIT_MASK(0x18000)
        , M_PLATFORM_ID(platform_id())
        , M_MODEL_NAME("Knights Landing")
        , M_TRIGGER_NAME("PKG_ENERGY_STATUS")
    {

    }

    KNLPlatformImp::KNLPlatformImp(const KNLPlatformImp &other)
        : PlatformImp(other)
        , m_throttle_limit_mhz(other.m_throttle_limit_mhz)
        , m_energy_units(other.m_energy_units)
        , m_power_units_inv(other.m_power_units_inv)
        , m_dram_energy_units(other.m_dram_energy_units)
        , m_min_pkg_watts(other.m_min_pkg_watts)
        , m_max_pkg_watts(other.m_max_pkg_watts)
        , m_min_dram_watts(other.m_min_dram_watts)
        , m_max_dram_watts(other.m_max_dram_watts)
        , m_control_msr_pair(other.m_control_msr_pair)
        , m_pkg_power_limit_static(other.m_pkg_power_limit_static)
        , M_BOX_FRZ_EN(other.M_BOX_FRZ_EN)
        , M_BOX_FRZ(other.M_BOX_FRZ)
        , M_CTR_EN(other.M_CTR_EN)
        , M_RST_CTRS(other.M_RST_CTRS)
        , M_DRAM_POWER_LIMIT_MASK(other.M_DRAM_POWER_LIMIT_MASK)
        , M_PLATFORM_ID(other.M_PLATFORM_ID)
        , M_MODEL_NAME(other.M_MODEL_NAME)
        , M_TRIGGER_NAME(other.M_TRIGGER_NAME)
    {

    }

    KNLPlatformImp::~KNLPlatformImp()
    {

    }

    bool KNLPlatformImp::is_model_supported(int platform_id)
    {
        return (platform_id == M_PLATFORM_ID);
    }

    std::string KNLPlatformImp::platform_name()
    {
        return M_MODEL_NAME;
    }

    int KNLPlatformImp::num_domain(int domain_type) const
    {
        int count;
        switch (domain_type) {
            case GEOPM_DOMAIN_SIGNAL_ENERGY:
            case GEOPM_DOMAIN_CONTROL_POWER:
            case GEOPM_DOMAIN_CONTROL_FREQUENCY:
                count = m_num_package;
                break;
            case GEOPM_DOMAIN_SIGNAL_PERF:
                count = m_num_logical_cpu;
                break;
            default:
                count = 0;
                break;
        }
        return count;
    }

    void KNLPlatformImp::create_domain_map(int domain, std::vector<std::vector<int> > &domain_map) const
    {
        if (domain == GEOPM_DOMAIN_SIGNAL_PERF) {
            domain_map.reserve(m_num_logical_cpu);
            std::vector<int> cpus;
            for (int i = 0; i < m_num_logical_cpu; ++i) {
                domain_map.push_back(std::vector<int>({i}));
            }
        }
        else if (domain == GEOPM_DOMAIN_SIGNAL_ENERGY ||
                 domain == GEOPM_DOMAIN_CONTROL_POWER  ||
                 domain == GEOPM_DOMAIN_CONTROL_FREQUENCY) {
            domain_map.reserve(m_num_package);
            std::vector<int> cpus(m_num_logical_cpu / m_num_package);
            for (int i = 0; i < m_num_package; ++i) {
                for (int j = i * m_num_hw_cpu; j < (i + 1) * m_num_hw_cpu; ++j) {
                    for (int k = 0; k < m_num_cpu_per_core; ++k) {
                        cpus.push_back(m_num_hw_cpu * k + j);
                    }
                }
                domain_map.push_back(cpus);
                cpus.clear();
            }
        }
        else {
            throw Exception("KNLPlatformImp::create_domain_maps() unknown domain type:" +
                            std::to_string(domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    double KNLPlatformImp::throttle_limit_mhz(void) const
    {
        return m_throttle_limit_mhz;
    }

    void KNLPlatformImp::batch_read_signal(std::vector<double> &signal_value)
    {
        std::vector<uint64_t> raw_val(m_msr_access->num_raw_signal());
        m_msr_access->read_batch(raw_val);
        auto value_it = signal_value.begin();
        auto batch_it = raw_val.begin();
        for (auto signal_it = m_signal.begin(); signal_it != m_signal.end(); ++signal_it) {
            for (int i = 0; i < (*signal_it)->num_source(); ++i) {
                std::vector<uint64_t> val;
                for (int j = 0; j < (*signal_it)->num_encoded(); ++j) {
                    val.push_back((*batch_it));
                    ++batch_it;
                }
                (*value_it) = (*signal_it)->sample(val);
                ++value_it;
            }
        }
    }

    void KNLPlatformImp::write_control(int control_domain, int domain_index, double value)
    {
        uint64_t msr_val = 0;
        int cpu_id = (m_num_hw_cpu / m_num_package) * domain_index;;

        switch (control_domain) {
            case GEOPM_DOMAIN_CONTROL_POWER:
                if (value < m_min_pkg_watts) {
                    value = m_min_pkg_watts;
                }
                if (value > m_max_pkg_watts) {
                    value = m_max_pkg_watts;
                }
                msr_val = (uint64_t)(value * m_power_units_inv);
                msr_val = msr_val | m_pkg_power_limit_static;
                m_msr_access->write(cpu_id, m_control_msr_pair[M_RAPL_PKG_LIMIT].first,
                          m_control_msr_pair[M_RAPL_PKG_LIMIT].second,  msr_val);
                break;
            case GEOPM_DOMAIN_CONTROL_FREQUENCY:
                msr_val = (uint64_t)(value * 10);
                msr_val = msr_val << 8;
                m_msr_access->write(cpu_id, m_control_msr_pair[M_IA32_PERF_CTL].first,
                          m_control_msr_pair[M_IA32_PERF_CTL].second,  msr_val);
                break;
            default:
                throw geopm::Exception("KNLPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
    }

    void KNLPlatformImp::msr_initialize(void)
    {
        rapl_init();
        m_trigger_offset = m_msr_access->offset(M_TRIGGER_NAME);
        // Get supported p-state bounds
        uint64_t tmp = m_msr_access->read(1, m_msr_access->offset("IA32_PLATFORM_INFO"));
        m_min_freq_mhz = ((tmp >> 40) | 0xFF) * 100.0;
        tmp = m_msr_access->read(1, m_msr_access->offset("TURBO_RATIO_LIMIT"));
        // This value is single core turbo
        m_max_freq_mhz = (tmp >> 8 | 0xFF) * 100.0;
        fixed_counters_init();
    }

    void KNLPlatformImp::init_telemetry(const TelemetryConfig &config)
    {
        std::vector<std::string> rapl_signals;
        std::vector<std::string> cpu_signals;
        config.get_required(GEOPM_DOMAIN_SIGNAL_ENERGY, rapl_signals);
        config.get_required(GEOPM_DOMAIN_SIGNAL_PERF, cpu_signals);
        std::vector<int> cpu;
        cpu.reserve(rapl_signals.size() * m_num_package +
                    cpu_signals.size() * m_num_logical_cpu);
        std::vector<int> read_off;
        read_off.reserve(rapl_signals.size() * m_num_package +
                         cpu_signals.size() * m_num_logical_cpu);
        for (auto it = rapl_signals.begin(); it != rapl_signals.end(); ++it) {
            auto signal_name = signal_to_msr_map.find(*it);
            if (signal_name == signal_to_msr_map.end()) {
                throw Exception("KNLPlatformImp::init_telemetry(): Invalid signal string", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            auto msr_entry = m_msr_signal_map_ptr->find((*signal_name).second);
            if (msr_entry == m_msr_signal_map_ptr->end()) {
                throw Exception("KNLPlatformImp::init_telemetry(): Invalid MSR type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            std::vector<off_t> off(1);
            off.back() = (*msr_entry).second.offset;
            m_signal.push_back(new MSRSignal(off, m_num_package));
            m_signal.back()->num_bit(0, (*msr_entry).second.size);
            m_signal.back()->left_shift(0, (*msr_entry).second.lshift_mod);
            m_signal.back()->right_shift(0, (*msr_entry).second.rshift_mod);
            m_signal.back()->mask(0, (*msr_entry).second.mask_mod);
            m_signal.back()->scalar(0, (*msr_entry).second.multiply_mod);
            for (int i = 0; i < m_num_package; ++i) {
                cpu.push_back((m_num_package / m_num_hw_cpu) * i);
                read_off.push_back(off.back());
            }
        }
        for (auto it = cpu_signals.begin(); it != cpu_signals.end(); ++it) {
            int counter_idx = 0;
            std::vector<off_t> off;
            std::vector<std::string> signame;
            std::vector<struct IMSRAccess::m_msr_signal_entry> msr_entry;
            std::string key;
            std::vector<std::vector<off_t> > per_cpu_offsets;
            std::string signal_string;
            if ((*it) == "bandwidth") {
                signame.push_back("llc_victims");
                signame.push_back("l2_prefetches");
            }
            else {
                signame.push_back(*it);
            }
            for (auto name_it = signame.begin(); name_it != signame.end(); ++name_it) {
                per_cpu_offsets.push_back(std::vector<off_t>(m_num_logical_cpu));
                auto signal_name = signal_to_msr_map.find(*name_it);
                if (signal_name == signal_to_msr_map.end()) {
                    throw Exception("KNLPlatformImp::init_telemetry(): Invalid signal string", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                key = (*signal_name).second;
                signal_string = key;
                if ((*signal_name).second.find("event-0x") == 0) {
                    uint32_t event = strtol((*signal_name).second.substr(std::string("event-0x").size()).c_str(), NULL, 16);
                    cbo_counters_init(counter_idx, event);
                    counter_idx++;
                    std::ostringstream buffer;
                    buffer << "C0_MSR_PMON_CTR" << counter_idx;
                    counter_idx++;
                    key = buffer.str();
                    int cpu_idx = 0;
                    int cpu_per_core = m_num_logical_cpu / m_num_hw_cpu;
                    int core_per_tile = m_num_hw_cpu / m_num_tile;
                    for (auto off_it = per_cpu_offsets.back().begin(); off_it != per_cpu_offsets.back().end(); ++off_it) {
                        int cha_idx = cpu_idx / (cpu_per_core * core_per_tile);
                        buffer.clear();
                        buffer << "C" << cha_idx << "_MSR_PMON_CTR" << counter_idx;
                        std::string lookup = buffer.str();
                        auto lookup_it = m_msr_signal_map_ptr->find(lookup);
                        if (lookup_it == m_msr_signal_map_ptr->end()) {
                            throw Exception("KNLPlatformImp::init_telemetry(): Invalid MSR type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                        }
                        (*off_it) = (*lookup_it).second.offset;
                        ++cpu_idx;
                    }
                }
                auto msr_it = m_msr_signal_map_ptr->find(key);
                if (msr_it == m_msr_signal_map_ptr->end()) {
                    throw Exception("KNLPlatformImp::init_telemetry(): Invalid MSR type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                msr_entry.push_back((*msr_it).second);
                off.push_back(msr_entry.back().offset);
                if (signal_string.find("event-0x") != 0) {
                     std::fill(per_cpu_offsets.back().begin(), per_cpu_offsets.back().end(), off.back());
                }
            }
            m_signal.push_back(new MSRSignal(off, m_num_logical_cpu));
            int count = 0;
            for (auto msr_entry_it = msr_entry.begin(); msr_entry_it != msr_entry.end(); ++msr_entry_it) {
                m_signal.back()->num_bit(count, msr_entry_it->size);
                m_signal.back()->left_shift(count, msr_entry_it->lshift_mod);
                m_signal.back()->right_shift(count, msr_entry_it->rshift_mod);
                m_signal.back()->mask(count, msr_entry_it->mask_mod);
                m_signal.back()->scalar(count, msr_entry_it->multiply_mod);
                count++;
            }

            for (int i = 0; i < m_num_logical_cpu; ++i) {
                for (auto off_it = per_cpu_offsets.begin(); off_it != per_cpu_offsets.end(); ++off_it) {
                    cpu.push_back(i);
                    read_off.push_back((*off_it)[i]);
                }
            }
        }

        //Save off the msr offsets and masks for the controls we want to write to avoid a map lookup
        m_control_msr_pair[M_RAPL_PKG_LIMIT] = std::make_pair(m_msr_access->offset("PKG_POWER_LIMIT"), m_msr_access->write_mask("PKG_POWER_LIMIT") );
        m_control_msr_pair[M_RAPL_DRAM_LIMIT] = std::make_pair(m_msr_access->offset("DRAM_POWER_LIMIT"), m_msr_access->write_mask("DRAM_POWER_LIMIT") );
        m_control_msr_pair[M_IA32_PERF_CTL] = std::make_pair(m_msr_access->offset("IA32_PERF_CTL"), m_msr_access->write_mask("IA32_PERF_CTL") );
    }

    double KNLPlatformImp::energy(void) const
    {
        double total_energy = 0;   
        for (int i = 0; i < m_num_package; ++i) {
            total_energy += msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_ENERGY_STATUS");
            total_energy += msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_ENERGY_STATUS");
        }
        return total_energy;    
    }

    void KNLPlatformImp::msr_reset()
    {
        cbo_counters_reset();
        fixed_counters_reset();
    }

    void KNLPlatformImp::rapl_init()
    {
        // RAPL_POWER_UNIT described in Section 14.9.1 of
        // Intel(R) 64 and IA-32 Architectures Software Developer’s
        // Manual Volume 3 (3A, 3B, 3C & 3D): System Programming Guide

        uint64_t tmp;

        //Make sure units are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT");
        m_power_units_inv = (double)(1 << (tmp & 0xF));
        m_energy_units = 1.0 / (double)(1 << ((tmp >> 8)  & 0x1F));
        double time_units = 1.0 / (double)(1 << ((tmp >> 16) & 0xF));

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "RAPL_POWER_UNIT");
            double power_inv = (double)(1 << (tmp & 0xF));
            double energy = 1.0 / (double)(1 << ((tmp >> 8)  & 0x1F));
            if (energy != m_energy_units || power_inv != m_power_units_inv) {
                throw Exception("detected inconsistent power units among packages",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        // PKG_POWER_INFO described in Section 14.9.3
        // Intel(R) 64 and IA-32 Architectures Software Developer’s
        // Manual Volume 3 (3A, 3B, 3C & 3D): System Programming Guide

        // Make sure bounds are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_INFO");
        m_tdp_pkg_watts = ((double)(tmp & 0x7fff)) / m_power_units_inv;
        m_min_pkg_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
        m_max_pkg_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;

        // DRAM_POWER_INFO described in Section 14.9.5
        // Intel(R) 64 and IA-32 Architectures Software Developer’s
        // Manual Volume 3 (3A, 3B, 3C & 3D): System Programming Guide

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "DRAM_POWER_INFO");
        m_min_dram_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
        m_max_dram_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;

        // PKG_POWER_LIMIT described in Section 14.9.3 of
        // Intel(R) 64 and IA-32 Architectures Software Developer’s
        // Manual Volume 3 (3A, 3B, 3C & 3D): System Programming Guide

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_LIMIT");
        // Set time window 1 to the minimum time window of 15 msec
        double tau = 0.015;
        uint64_t pkg_time_window_y = (uint64_t)std::log2(tau/time_units);
        uint64_t pkg_time_window_z = (uint64_t)(4.0 * ((tau / ((double)(1 << pkg_time_window_y) * time_units)) - 1.0));
        if ((pkg_time_window_z >> 2) != 0 || (pkg_time_window_y >> 5) != 0) {
            throw Exception("KNLPlatformImp::rapl_init(): Package time limit too large",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        double tau_inferred = (1 << pkg_time_window_y) * (1.0 + (pkg_time_window_z / 4.0)) * time_units;
        if ((tau - tau_inferred) > (tau  / 4.0)) {
            throw Exception("KNLPlatformImp::rapl_init(): Time window calculation inaccurate: "
                            + std::to_string(tau_inferred),
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        pkg_time_window_y = pkg_time_window_y << 17;
        pkg_time_window_z = pkg_time_window_z << 22;
        m_pkg_power_limit_static = (tmp & 0x00FFFFFF00FF0000) | pkg_time_window_y | pkg_time_window_z;
        // enable pl1 limits
        m_pkg_power_limit_static = m_pkg_power_limit_static | (0x3 << 15);

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_POWER_INFO");
            double pkg_min = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
            double pkg_max = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;
            if (pkg_min != m_min_pkg_watts || pkg_max != m_max_pkg_watts) {
                throw Exception("detected inconsistent power pkg bounds among packages",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
            double dram_min = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
            double dram_max = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;
            if (dram_min != m_min_dram_watts || dram_max != m_max_dram_watts) {
                throw Exception("detected inconsistent power dram bounds among packages",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void KNLPlatformImp::cbo_counters_init(int counter_idx, uint32_t event)
    {
        for (int i = 0; i < m_num_tile; i++) {
            std::string ctl_msr_name("_MSR_PMON_CTL" + std::to_string(counter_idx));
            std::string box_msr_name("_MSR_PMON_BOX_CTL");
            box_msr_name.insert(0, std::to_string(i));
            box_msr_name.insert(0, "C");
            ctl_msr_name.insert(0, std::to_string(i));
            ctl_msr_name.insert(0, "C");

            // enable freeze
            msr_write(GEOPM_DOMAIN_TILE, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, box_msr_name)
                      | M_BOX_FRZ_EN);
            // freeze box
            msr_write(GEOPM_DOMAIN_TILE, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, box_msr_name)
                      | M_BOX_FRZ);
            // enable counter
            msr_write(GEOPM_DOMAIN_TILE, i, ctl_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, ctl_msr_name)
                      | M_CTR_EN);
            // program event
            msr_write(GEOPM_DOMAIN_TILE, i, ctl_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, ctl_msr_name)
                      | event);
            // reset counters
            msr_write(GEOPM_DOMAIN_TILE, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, box_msr_name)
                      | M_RST_CTRS);
            // disable freeze
            msr_write(GEOPM_DOMAIN_TILE, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, box_msr_name)
                      | M_BOX_FRZ);
            // unfreeze box
            msr_write(GEOPM_DOMAIN_TILE, i, box_msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, box_msr_name)
                      & ~M_BOX_FRZ_EN);
        }
    }

    void KNLPlatformImp::fixed_counters_init()
    {
        for (int tile = 0; tile < m_num_tile; tile++) {
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_FIXED_CTR_CTRL", 0x0333);
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_GLOBAL_CTRL", 0x700000003);
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void KNLPlatformImp::cbo_counters_reset()
    {
        for (int i = 0; i < m_num_tile; i++) {
            std::string msr_name("_MSR_PMON_BOX_CTL");
            msr_name.insert(0, std::to_string(i));
            msr_name.insert(0, "C");
            // reset counters
            msr_write(GEOPM_DOMAIN_TILE, i, msr_name,
                      msr_read(GEOPM_DOMAIN_TILE, i, msr_name)
                      | M_RST_CTRS);
        }
    }

    void KNLPlatformImp::fixed_counters_reset()
    {
        for (int tile = 0; tile < m_num_tile; tile++) {
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_FIXED_CTR0", 0x0);
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_FIXED_CTR1", 0x0);
            msr_write(GEOPM_DOMAIN_TILE, tile, "PERF_FIXED_CTR2", 0x0);
        }
    }

    void KNLPlatformImp::provides(TelemetryConfig &config) const
    {
        std::vector<int> domains = {GEOPM_DOMAIN_CONTROL_POWER, GEOPM_DOMAIN_CONTROL_FREQUENCY,
                                    GEOPM_DOMAIN_SIGNAL_ENERGY, GEOPM_DOMAIN_SIGNAL_PERF};
        std::vector<std::string> energy_signals = {"dram_energy", "pkg_energy"};
        std::vector<std::string> counter_signals = {"frequency", "instructions_retired", "clock_unhalted_core", "clock_unhalted_ref", "read_bandwidth"};
        std::vector<std::vector<int> > domain_map;
        config.supported_domain(domains);
        config.set_provided(GEOPM_DOMAIN_SIGNAL_ENERGY, energy_signals);
        config.set_provided(GEOPM_DOMAIN_SIGNAL_PERF, counter_signals);
        config.set_bounds(GEOPM_DOMAIN_CONTROL_POWER, m_min_pkg_watts + m_max_dram_watts, m_max_pkg_watts + m_max_dram_watts);
        config.set_bounds(GEOPM_DOMAIN_CONTROL_FREQUENCY, m_min_freq_mhz, m_max_freq_mhz);
        create_domain_map(GEOPM_DOMAIN_CONTROL_POWER, domain_map);
        config.set_domain_cpu_map(GEOPM_DOMAIN_CONTROL_POWER, domain_map);
        domain_map.clear();
        create_domain_map(GEOPM_DOMAIN_CONTROL_FREQUENCY, domain_map);
        config.set_domain_cpu_map(GEOPM_DOMAIN_CONTROL_FREQUENCY, domain_map);
        domain_map.clear();
        create_domain_map(GEOPM_DOMAIN_SIGNAL_ENERGY, domain_map);
        config.set_domain_cpu_map(GEOPM_DOMAIN_SIGNAL_ENERGY, domain_map);
        domain_map.clear();
        create_domain_map(GEOPM_DOMAIN_SIGNAL_PERF, domain_map);
        config.set_domain_cpu_map(GEOPM_DOMAIN_SIGNAL_PERF, domain_map);
    }

    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &knl_msr_signal_map(void)
    {
        static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> msr_signal_map({
            {"IA32_PERF_STATUS",        {0x0198, 0x0000000000000000, 32, 0, 8, 0x0ff, 0.1}},
            {"PKG_ENERGY_STATUS",       {0x0611, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"DRAM_ENERGY_STATUS",      {0x0619, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR0",         {0x0309, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR1",         {0x030A, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR2",         {0x030B, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR0",        {0x0E08, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR0",        {0x0E14, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR0",        {0x0E20, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR0",        {0x0E2C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR0",        {0x0E38, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR0",        {0x0E44, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR0",        {0x0E50, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR0",        {0x0E5C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR0",        {0x0E68, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR0",        {0x0E74, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR0",       {0x0E80, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR0",       {0x0E8C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR0",       {0x0E98, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR0",       {0x0EA4, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR0",       {0x0EB0, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C15_MSR_PMON_CTR0",       {0x0EBC, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C16_MSR_PMON_CTR0",       {0x0EC8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C17_MSR_PMON_CTR0",       {0x0ED4, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C18_MSR_PMON_CTR0",       {0x0EE0, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C19_MSR_PMON_CTR0",       {0x0EEC, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C20_MSR_PMON_CTR0",       {0x0EF8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C21_MSR_PMON_CTR0",       {0x0F04, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C22_MSR_PMON_CTR0",       {0x0F10, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C23_MSR_PMON_CTR0",       {0x0F1C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C24_MSR_PMON_CTR0",       {0x0F28, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C25_MSR_PMON_CTR0",       {0x0F34, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C26_MSR_PMON_CTR0",       {0x0F40, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C27_MSR_PMON_CTR0",       {0x0F4C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C28_MSR_PMON_CTR0",       {0x0F58, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C29_MSR_PMON_CTR0",       {0x0F64, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C30_MSR_PMON_CTR0",       {0x0F70, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C31_MSR_PMON_CTR0",       {0x0F7C, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C32_MSR_PMON_CTR0",       {0x0F88, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C33_MSR_PMON_CTR0",       {0x0F94, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C34_MSR_PMON_CTR0",       {0x0FA0, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C35_MSR_PMON_CTR0",       {0x0FAC, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C36_MSR_PMON_CTR0",       {0x0FB8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C37_MSR_PMON_CTR0",       {0x0FC4, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR1",        {0x0E09, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR1",        {0x0E15, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR1",        {0x0E21, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR1",        {0x0E2D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR1",        {0x0E39, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR1",        {0x0E45, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR1",        {0x0E51, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR1",        {0x0E5D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR1",        {0x0E69, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR1",        {0x0E75, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR1",       {0x0E81, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR1",       {0x0E8D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR1",       {0x0E99, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR1",       {0x0EA5, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR1",       {0x0EB1, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C15_MSR_PMON_CTR1",       {0x0EBD, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C16_MSR_PMON_CTR1",       {0x0EC9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C17_MSR_PMON_CTR1",       {0x0ED5, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C17_MSR_PMON_CTR1",       {0x0EE1, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C18_MSR_PMON_CTR1",       {0x0EED, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C19_MSR_PMON_CTR1",       {0x0EF9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C20_MSR_PMON_CTR1",       {0x0F05, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C21_MSR_PMON_CTR1",       {0x0F11, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C22_MSR_PMON_CTR1",       {0x0F1D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C23_MSR_PMON_CTR1",       {0x0F29, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C24_MSR_PMON_CTR1",       {0x0F35, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C25_MSR_PMON_CTR1",       {0x0F41, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C26_MSR_PMON_CTR1",       {0x0F4D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C27_MSR_PMON_CTR1",       {0x0F59, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C28_MSR_PMON_CTR1",       {0x0F65, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C29_MSR_PMON_CTR1",       {0x0F71, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C30_MSR_PMON_CTR1",       {0x0F7D, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C31_MSR_PMON_CTR1",       {0x0F89, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C32_MSR_PMON_CTR1",       {0x0F95, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C33_MSR_PMON_CTR1",       {0x0FA1, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C34_MSR_PMON_CTR1",       {0x0FAD, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C35_MSR_PMON_CTR1",       {0x0FB9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C37_MSR_PMON_CTR1",       {0x0FC5, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}}
        });
        return msr_signal_map;
    }

    static const std::map<std::string, std::pair<off_t, uint64_t> > &knl_msr_control_map(void)
    {
        static const std::map<std::string, std::pair<off_t, uint64_t> > msr_control_map({
            {"IA32_PLATFORM_INFO",      {0x00CE, 0x0000000000000000}},
            {"IA32_PERF_CTL",           {0x0199, 0x000000010000ffff}},
            {"TURBO_RATIO_LIMIT",       {0x01AD, 0x0000000000000000}},
            {"RAPL_POWER_UNIT",         {0x0606, 0x0000000000000000}},
            {"PKG_POWER_LIMIT",         {0x0610, 0x00ffffff00ffffff}},
            {"PKG_POWER_INFO",          {0x0614, 0x0000000000000000}},
            {"DRAM_POWER_LIMIT",        {0x0618, 0x0000000000ffffff}},
            {"DRAM_PERF_STATUS",        {0x061B, 0x0000000000000000}},
            {"DRAM_POWER_INFO",         {0x061C, 0x0000000000000000}},
            {"PERF_FIXED_CTR_CTRL",     {0x038D, 0x0000000000000bbb}},
            {"PERF_GLOBAL_CTRL",        {0x038F, 0x0000000700000003}},
            {"PERF_GLOBAL_OVF_CTRL",    {0x0390, 0xc000000700000003}},
            {"C0_MSR_PMON_BOX_CTL",     {0x0E00, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_CTL",     {0x0E0C, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_CTL",     {0x0E18, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_CTL",     {0x0E24, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_CTL",     {0x0E30, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_CTL",     {0x0E3C, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_CTL",     {0x0E48, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_CTL",     {0x0E54, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_CTL",     {0x0E60, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_CTL",     {0x0E6C, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_CTL",    {0x0E78, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_CTL",    {0x0E84, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_CTL",    {0x0E90, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_CTL",    {0x0E9C, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_CTL",    {0x0EA8, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_CTL",    {0x0EB4, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_CTL",    {0x0EC0, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_CTL",    {0x0ECC, 0x00000000ffffffff}},
            {"C18_MSR_PMON_BOX_CTL",    {0x0ED8, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_CTL",    {0x0EE4, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_CTL",    {0x0EF0, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_CTL",    {0x0EFC, 0x00000000ffffffff}},
            {"C22_MSR_PMON_BOX_CTL",    {0x0F08, 0x00000000ffffffff}},
            {"C23_MSR_PMON_BOX_CTL",    {0x0F14, 0x00000000ffffffff}},
            {"C24_MSR_PMON_BOX_CTL",    {0x0F20, 0x00000000ffffffff}},
            {"C25_MSR_PMON_BOX_CTL",    {0x0F2C, 0x00000000ffffffff}},
            {"C26_MSR_PMON_BOX_CTL",    {0x0F38, 0x00000000ffffffff}},
            {"C27_MSR_PMON_BOX_CTL",    {0x0F44, 0x00000000ffffffff}},
            {"C28_MSR_PMON_BOX_CTL",    {0x0F50, 0x00000000ffffffff}},
            {"C29_MSR_PMON_BOX_CTL",    {0x0F5C, 0x00000000ffffffff}},
            {"C30_MSR_PMON_BOX_CTL",    {0x0F68, 0x00000000ffffffff}},
            {"C31_MSR_PMON_BOX_CTL",    {0x0F74, 0x00000000ffffffff}},
            {"C32_MSR_PMON_BOX_CTL",    {0x0F80, 0x00000000ffffffff}},
            {"C33_MSR_PMON_BOX_CTL",    {0x0F8C, 0x00000000ffffffff}},
            {"C34_MSR_PMON_BOX_CTL",    {0x0F98, 0x00000000ffffffff}},
            {"C35_MSR_PMON_BOX_CTL",    {0x0FA4, 0x00000000ffffffff}},
            {"C36_MSR_PMON_BOX_CTL",    {0x0FB0, 0x00000000ffffffff}},
            {"C37_MSR_PMON_BOX_CTL",    {0x0FBC, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER",  {0x0E05, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER",  {0x0E11, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER",  {0x0E1D, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER",  {0x0E29, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER",  {0x0E35, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER",  {0x0E41, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER",  {0x0E4D, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER",  {0x0E59, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER",  {0x0E65, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER",  {0x0E71, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER", {0x0E7D, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER", {0x0E89, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER", {0x0E95, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER", {0x0EA1, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER", {0x0EAD, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_FILTER", {0x0EB9, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_FILTER", {0x0EC5, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_FILTER", {0x0ED1, 0x00000000ffffffff}},
            {"C18_MSR_PMON_BOX_FILTER", {0x0EDD, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_FILTER", {0x0EE9, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_FILTER", {0x0EF5, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_FILTER", {0x0F01, 0x00000000ffffffff}},
            {"C22_MSR_PMON_BOX_FILTER", {0x0F0D, 0x00000000ffffffff}},
            {"C23_MSR_PMON_BOX_FILTER", {0x0F19, 0x00000000ffffffff}},
            {"C24_MSR_PMON_BOX_FILTER", {0x0F25, 0x00000000ffffffff}},
            {"C25_MSR_PMON_BOX_FILTER", {0x0F31, 0x00000000ffffffff}},
            {"C26_MSR_PMON_BOX_FILTER", {0x0F3D, 0x00000000ffffffff}},
            {"C27_MSR_PMON_BOX_FILTER", {0x0F49, 0x00000000ffffffff}},
            {"C28_MSR_PMON_BOX_FILTER", {0x0F55, 0x00000000ffffffff}},
            {"C29_MSR_PMON_BOX_FILTER", {0x0F61, 0x00000000ffffffff}},
            {"C30_MSR_PMON_BOX_FILTER", {0x0F6D, 0x00000000ffffffff}},
            {"C31_MSR_PMON_BOX_FILTER", {0x0F79, 0x00000000ffffffff}},
            {"C32_MSR_PMON_BOX_FILTER", {0x0F85, 0x00000000ffffffff}},
            {"C33_MSR_PMON_BOX_FILTER", {0x0F91, 0x00000000ffffffff}},
            {"C34_MSR_PMON_BOX_FILTER", {0x0F9D, 0x00000000ffffffff}},
            {"C35_MSR_PMON_BOX_FILTER", {0x0FA9, 0x00000000ffffffff}},
            {"C36_MSR_PMON_BOX_FILTER", {0x0FB5, 0x00000000ffffffff}},
            {"C37_MSR_PMON_BOX_FILTER", {0x0FC1, 0x00000000ffffffff}},
            {"C0_MSR_PMON_BOX_FILTER1", {0x0E06, 0x00000000ffffffff}},
            {"C1_MSR_PMON_BOX_FILTER1", {0x0E12, 0x00000000ffffffff}},
            {"C2_MSR_PMON_BOX_FILTER1", {0x0E1E, 0x00000000ffffffff}},
            {"C3_MSR_PMON_BOX_FILTER1", {0x0E2A, 0x00000000ffffffff}},
            {"C4_MSR_PMON_BOX_FILTER1", {0x0E36, 0x00000000ffffffff}},
            {"C5_MSR_PMON_BOX_FILTER1", {0x0E42, 0x00000000ffffffff}},
            {"C6_MSR_PMON_BOX_FILTER1", {0x0E4E, 0x00000000ffffffff}},
            {"C7_MSR_PMON_BOX_FILTER1", {0x0E5A, 0x00000000ffffffff}},
            {"C8_MSR_PMON_BOX_FILTER1", {0x0E66, 0x00000000ffffffff}},
            {"C9_MSR_PMON_BOX_FILTER1", {0x0E72, 0x00000000ffffffff}},
            {"C10_MSR_PMON_BOX_FILTER1",{0x0E7E, 0x00000000ffffffff}},
            {"C11_MSR_PMON_BOX_FILTER1",{0x0E8A, 0x00000000ffffffff}},
            {"C12_MSR_PMON_BOX_FILTER1",{0x0E96, 0x00000000ffffffff}},
            {"C13_MSR_PMON_BOX_FILTER1",{0x0EA2, 0x00000000ffffffff}},
            {"C14_MSR_PMON_BOX_FILTER1",{0x0EAE, 0x00000000ffffffff}},
            {"C15_MSR_PMON_BOX_FILTER1",{0x0EBA, 0x00000000ffffffff}},
            {"C16_MSR_PMON_BOX_FILTER1",{0x0EC6, 0x00000000ffffffff}},
            {"C17_MSR_PMON_BOX_FILTER1",{0x0ED2, 0x00000000ffffffff}},
            {"C18_MSR_PMON_BOX_FILTER1",{0x0EDE, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_FILTER1",{0x0EEA, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_FILTER1",{0x0EF6, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_FILTER1",{0x0F02, 0x00000000ffffffff}},
            {"C22_MSR_PMON_BOX_FILTER1",{0x0F0E, 0x00000000ffffffff}},
            {"C23_MSR_PMON_BOX_FILTER1",{0x0F1A, 0x00000000ffffffff}},
            {"C24_MSR_PMON_BOX_FILTER1",{0x0F26, 0x00000000ffffffff}},
            {"C25_MSR_PMON_BOX_FILTER1",{0x0F32, 0x00000000ffffffff}},
            {"C26_MSR_PMON_BOX_FILTER1",{0x0F3E, 0x00000000ffffffff}},
            {"C27_MSR_PMON_BOX_FILTER1",{0x0F4A, 0x00000000ffffffff}},
            {"C28_MSR_PMON_BOX_FILTER1",{0x0F56, 0x00000000ffffffff}},
            {"C29_MSR_PMON_BOX_FILTER1",{0x0F62, 0x00000000ffffffff}},
            {"C30_MSR_PMON_BOX_FILTER1",{0x0F6E, 0x00000000ffffffff}},
            {"C31_MSR_PMON_BOX_FILTER1",{0x0F7A, 0x00000000ffffffff}},
            {"C32_MSR_PMON_BOX_FILTER1",{0x0F86, 0x00000000ffffffff}},
            {"C33_MSR_PMON_BOX_FILTER1",{0x0F92, 0x00000000ffffffff}},
            {"C34_MSR_PMON_BOX_FILTER1",{0x0F9E, 0x00000000ffffffff}},
            {"C35_MSR_PMON_BOX_FILTER1",{0x0FAA, 0x00000000ffffffff}},
            {"C36_MSR_PMON_BOX_FILTER1",{0x0FB6, 0x00000000ffffffff}},
            {"C37_MSR_PMON_BOX_FILTER1",{0x0FC2, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL0",        {0x0E01, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL0",        {0x0E0D, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL0",        {0x0E19, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL0",        {0x0E25, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL0",        {0x0E31, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL0",        {0x0E3D, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL0",        {0x0E49, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL0",        {0x0E55, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL0",        {0x0E61, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL0",        {0x0E6D, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL0",       {0x0E79, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL0",       {0x0E85, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL0",       {0x0E91, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL0",       {0x0E9D, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL0",       {0x0EA9, 0x00000000ffffffff}},
            {"C15_MSR_PMON_CTL0",       {0x0EB5, 0x00000000ffffffff}},
            {"C16_MSR_PMON_CTL0",       {0x0EC1, 0x00000000ffffffff}},
            {"C17_MSR_PMON_CTL0",       {0x0ECD, 0x00000000ffffffff}},
            {"C18_MSR_PMON_CTL0",       {0x0ED9, 0x00000000ffffffff}},
            {"C19_MSR_PMON_CTL0",       {0x0EE5, 0x00000000ffffffff}},
            {"C20_MSR_PMON_CTL0",       {0x0EF1, 0x00000000ffffffff}},
            {"C21_MSR_PMON_CTL0",       {0x0EFD, 0x00000000ffffffff}},
            {"C22_MSR_PMON_CTL0",       {0x0F09, 0x00000000ffffffff}},
            {"C23_MSR_PMON_CTL0",       {0x0F15, 0x00000000ffffffff}},
            {"C24_MSR_PMON_CTL0",       {0x0F21, 0x00000000ffffffff}},
            {"C25_MSR_PMON_CTL0",       {0x0F2D, 0x00000000ffffffff}},
            {"C26_MSR_PMON_CTL0",       {0x0F39, 0x00000000ffffffff}},
            {"C27_MSR_PMON_CTL0",       {0x0F45, 0x00000000ffffffff}},
            {"C28_MSR_PMON_CTL0",       {0x0F51, 0x00000000ffffffff}},
            {"C29_MSR_PMON_CTL0",       {0x0F5D, 0x00000000ffffffff}},
            {"C30_MSR_PMON_CTL0",       {0x0F69, 0x00000000ffffffff}},
            {"C31_MSR_PMON_CTL0",       {0x0F75, 0x00000000ffffffff}},
            {"C32_MSR_PMON_CTL0",       {0x0F81, 0x00000000ffffffff}},
            {"C33_MSR_PMON_CTL0",       {0x0F8D, 0x00000000ffffffff}},
            {"C34_MSR_PMON_CTL0",       {0x0F99, 0x00000000ffffffff}},
            {"C35_MSR_PMON_CTL0",       {0x0FA5, 0x00000000ffffffff}},
            {"C36_MSR_PMON_CTL0",       {0x0FB1, 0x00000000ffffffff}},
            {"C37_MSR_PMON_CTL0",       {0x0FBD, 0x00000000ffffffff}},
            {"C0_MSR_PMON_CTL1",        {0x0E02, 0x00000000ffffffff}},
            {"C1_MSR_PMON_CTL1",        {0x0E0E, 0x00000000ffffffff}},
            {"C2_MSR_PMON_CTL1",        {0x0E1A, 0x00000000ffffffff}},
            {"C3_MSR_PMON_CTL1",        {0x0E26, 0x00000000ffffffff}},
            {"C4_MSR_PMON_CTL1",        {0x0E32, 0x00000000ffffffff}},
            {"C5_MSR_PMON_CTL1",        {0x0E3E, 0x00000000ffffffff}},
            {"C6_MSR_PMON_CTL1",        {0x0E4A, 0x00000000ffffffff}},
            {"C7_MSR_PMON_CTL1",        {0x0E56, 0x00000000ffffffff}},
            {"C8_MSR_PMON_CTL1",        {0x0E62, 0x00000000ffffffff}},
            {"C9_MSR_PMON_CTL1",        {0x0E6E, 0x00000000ffffffff}},
            {"C10_MSR_PMON_CTL1",       {0x0E7A, 0x00000000ffffffff}},
            {"C11_MSR_PMON_CTL1",       {0x0E86, 0x00000000ffffffff}},
            {"C12_MSR_PMON_CTL1",       {0x0E92, 0x00000000ffffffff}},
            {"C13_MSR_PMON_CTL1",       {0x0E9E, 0x00000000ffffffff}},
            {"C14_MSR_PMON_CTL1",       {0x0EAA, 0x00000000ffffffff}},
            {"C15_MSR_PMON_CTL1",       {0x0EB6, 0x00000000ffffffff}},
            {"C16_MSR_PMON_CTL1",       {0x0EC2, 0x00000000ffffffff}},
            {"C17_MSR_PMON_CTL1",       {0x0ECE, 0x00000000ffffffff}},
            {"C18_MSR_PMON_CTL1",       {0x0EDA, 0x00000000ffffffff}},
            {"C19_MSR_PMON_CTL1",       {0x0EE6, 0x00000000ffffffff}},
            {"C20_MSR_PMON_CTL1",       {0x0EF2, 0x00000000ffffffff}},
            {"C21_MSR_PMON_CTL1",       {0x0EFE, 0x00000000ffffffff}},
            {"C22_MSR_PMON_CTL1",       {0x0F0A, 0x00000000ffffffff}},
            {"C23_MSR_PMON_CTL1",       {0x0F16, 0x00000000ffffffff}},
            {"C24_MSR_PMON_CTL1",       {0x0F22, 0x00000000ffffffff}},
            {"C25_MSR_PMON_CTL1",       {0x0F2E, 0x00000000ffffffff}},
            {"C26_MSR_PMON_CTL1",       {0x0F3A, 0x00000000ffffffff}},
            {"C27_MSR_PMON_CTL1",       {0x0F46, 0x00000000ffffffff}},
            {"C28_MSR_PMON_CTL1",       {0x0F52, 0x00000000ffffffff}},
            {"C29_MSR_PMON_CTL1",       {0x0F5E, 0x00000000ffffffff}},
            {"C30_MSR_PMON_CTL1",       {0x0F6A, 0x00000000ffffffff}},
            {"C31_MSR_PMON_CTL1",       {0x0F76, 0x00000000ffffffff}},
            {"C32_MSR_PMON_CTL1",       {0x0F82, 0x00000000ffffffff}},
            {"C33_MSR_PMON_CTL1",       {0x0F8E, 0x00000000ffffffff}},
            {"C34_MSR_PMON_CTL1",       {0x0F9A, 0x00000000ffffffff}},
            {"C35_MSR_PMON_CTL1",       {0x0FA6, 0x00000000ffffffff}},
            {"C36_MSR_PMON_CTL1",       {0x0FB2, 0x00000000ffffffff}},
            {"C37_MSR_PMON_CTL1",       {0x0FBE, 0x00000000ffffffff}}
        });
        return msr_control_map;
    }
}
