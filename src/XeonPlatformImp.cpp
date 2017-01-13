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
#include <set>
#include <unistd.h>

#include "geopm_message.h"
#include "Exception.hpp"
#include "XeonPlatformImp.hpp"
#include "config.h"

namespace geopm
{
    static const std::map<std::string, std::pair<off_t, uint64_t> > &snb_msr_control_map(void);
    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &snb_msr_signal_map(void);
    static const std::map<std::string, std::pair<off_t, uint64_t> > &hsx_msr_control_map(void);
    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &hsx_msr_signal_map(void);
    static const std::map<std::string, std::string> signal_to_msr_map {
        {"pkg_energy", "PKG_ENERGY_STATUS"},
        {"dram_energy", "DRAM_ENERGY_STATUS"},
        {"frequency", "IA32_PERF_STATUS"},
        {"instructions_retired", "PERF_FIXED_CTR0"},
        {"clock_unhalted_core", "PERF_FIXED_CTR1"},
        {"clock_unhalted_ref", "PERF_FIXED_CTR2"},
        {"bandwidth", "event-0x737"}
    };

    XeonPlatformImp::XeonPlatformImp(int platform_id, const std::string &model_name,
                                     const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> *msr_signal_map,
                                     const std::map<std::string, std::pair<off_t, uint64_t> > *msr_control_map)
        : PlatformImp({{GEOPM_DOMAIN_CONTROL_POWER, 50.0},{GEOPM_DOMAIN_CONTROL_FREQUENCY, 1.0}}, msr_signal_map, msr_control_map)
        , m_throttle_limit_mhz(0.5)
        , m_energy_units(0.0)
        , m_dram_energy_units(0.0)
        , m_power_units_inv(0.0)
        , m_min_pkg_watts(1.0)
        , m_max_pkg_watts(100.0)
        , m_min_dram_watts(1.0)
        , m_max_dram_watts(100.0)
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
        , M_PLATFORM_ID(platform_id)
        , M_MODEL_NAME(model_name)
        , M_TRIGGER_NAME("PKG_ENERGY_STATUS")
    {

    }

    XeonPlatformImp::XeonPlatformImp(const XeonPlatformImp &other)
        : PlatformImp(other)
        , m_throttle_limit_mhz(other.m_throttle_limit_mhz)
        , m_energy_units(other.m_energy_units)
        , m_dram_energy_units(other.m_dram_energy_units)
        , m_power_units_inv(other.m_power_units_inv)
        , m_min_pkg_watts(other.m_min_pkg_watts)
        , m_max_pkg_watts(other.m_max_pkg_watts)
        , m_min_dram_watts(other.m_min_dram_watts)
        , m_max_dram_watts(other.m_max_dram_watts)
        , m_min_freq_mhz(other.m_min_freq_mhz)
        , m_max_freq_mhz(other.m_max_freq_mhz)
        , m_freq_step_mhz(other.m_freq_step_mhz)
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

    XeonPlatformImp::~XeonPlatformImp()
    {

    }

    int SNBPlatformImp::platform_id(void)
    {
        return 0x62D;
    }

    SNBPlatformImp::SNBPlatformImp()
        : XeonPlatformImp(platform_id(), "Sandybridge E", &(snb_msr_signal_map()), &(snb_msr_control_map()))
    {
        // Get supported p-state bounds
        uint64_t tmp = m_msr_access->read(0, m_msr_access->offset("IA32_PLATFORM_INFO"));
        m_min_freq_mhz = ((tmp >> 40) | 0xFF) * 100.0;
        m_max_freq_mhz = (tmp | 0xFF) * 100.0;
    }

    SNBPlatformImp::SNBPlatformImp(int platform_id, const std::string &model_name)
        : XeonPlatformImp(platform_id, model_name, &(snb_msr_signal_map()), &(snb_msr_control_map()))
    {

    }

    SNBPlatformImp::SNBPlatformImp(const SNBPlatformImp &other)
        : XeonPlatformImp(other)
    {

    }


    SNBPlatformImp::~SNBPlatformImp()
    {

    }

    int SNBPlatformImp::num_domain(int domain_type) const
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

    void SNBPlatformImp::create_domain_map(int domain, std::vector<std::vector<int> > &domain_map) const
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
            throw Exception("SNBPlatformImp::create_domain_maps() unknown domain type:" +
                            std::to_string(domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void SNBPlatformImp::provides(TelemetryConfig &config) const
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

    int IVTPlatformImp::platform_id(void)
    {
        return 0x63E;
    }

    IVTPlatformImp::IVTPlatformImp()
        : SNBPlatformImp(platform_id(), "Ivybridge E")
    {

    }

    IVTPlatformImp::IVTPlatformImp(const IVTPlatformImp &other)
        : SNBPlatformImp(other)
    {

    }

    IVTPlatformImp::~IVTPlatformImp()
    {

    }

    int HSXPlatformImp::platform_id(void)
    {
        return 0x63F;
    }

    HSXPlatformImp::HSXPlatformImp()
        : XeonPlatformImp(platform_id(), "Haswell E", &(hsx_msr_signal_map()), &(hsx_msr_control_map()))
    {
        XeonPlatformImp::m_dram_energy_units = 1.5258789063E-5;
    }

    HSXPlatformImp::HSXPlatformImp(int platform_id, const std::string &model_name)
        : XeonPlatformImp(platform_id, model_name, &(hsx_msr_signal_map()), &(hsx_msr_control_map()))
    {
        XeonPlatformImp::m_dram_energy_units = 1.5258789063E-5;

        // Get supported p-state bounds
        uint64_t tmp = m_msr_access->read(0, m_msr_access->offset("IA32_PLATFORM_INFO"));
        m_min_freq_mhz = (tmp >> 40) | 0xFF;
        tmp = m_msr_access->read(0, m_msr_access->offset("TURBO_RATIO_LIMIT"));
        // This value is single core turbo
        m_max_freq_mhz = (tmp | 0xFF) * 100.0;
    }

    HSXPlatformImp::HSXPlatformImp(const HSXPlatformImp &other)
        : XeonPlatformImp(other)
    {

    }

    HSXPlatformImp::~HSXPlatformImp()
    {

    }

    int HSXPlatformImp::num_domain(int domain_type) const
    {
        int count;
        switch (domain_type) {
            case GEOPM_DOMAIN_SIGNAL_ENERGY:
            case GEOPM_DOMAIN_CONTROL_POWER:
                count = m_num_package;
                break;
            case GEOPM_DOMAIN_CONTROL_FREQUENCY:
            case GEOPM_DOMAIN_SIGNAL_PERF:
                count = m_num_logical_cpu;
                break;
            default:
                count = 0;
                break;
        }
        return count;
    }

    int BDXPlatformImp::platform_id(void)
    {
        return 0x64F;
    }

    void HSXPlatformImp::create_domain_map(int domain, std::vector<std::vector<int> > &domain_map) const
    {
        if (domain == GEOPM_DOMAIN_SIGNAL_PERF ||
            domain == GEOPM_DOMAIN_CONTROL_FREQUENCY) {
            domain_map.reserve(m_num_logical_cpu);
            std::vector<int> cpus;
            for (int i = 0; i < m_num_logical_cpu; ++i) {
                domain_map.push_back(std::vector<int>({i}));
            }
        }
        else if (domain == GEOPM_DOMAIN_SIGNAL_ENERGY ||
                 domain == GEOPM_DOMAIN_CONTROL_POWER) {
            domain_map.reserve(m_num_package);
            std::vector<int> cpus;
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
            throw Exception("HSXPlatformImp::create_domain_maps() unknown domain type:" +
                            std::to_string(domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void HSXPlatformImp::provides(TelemetryConfig &config) const
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

    BDXPlatformImp::BDXPlatformImp()
        : HSXPlatformImp(platform_id(), "Broadwell E")
    {

    }

    BDXPlatformImp::BDXPlatformImp(const BDXPlatformImp &other)
        : HSXPlatformImp(other)
    {

    }

    BDXPlatformImp::~BDXPlatformImp()
    {

    }

    bool XeonPlatformImp::is_model_supported(int platform_id)
    {
        return (platform_id == M_PLATFORM_ID);
    }

    std::string XeonPlatformImp::platform_name()
    {
        return M_MODEL_NAME;
    }

    double XeonPlatformImp::throttle_limit_mhz(void) const
    {
        return m_throttle_limit_mhz;
    }

    void XeonPlatformImp::batch_read_signal(std::vector<double> &signal_value)
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

    void XeonPlatformImp::write_control(int control_domain, int domain_index, double value)
    {
        uint64_t msr_val = 0;
        int cpu_id = domain_index;

        switch (control_domain) {
            case GEOPM_DOMAIN_CONTROL_POWER:
                cpu_id = (m_num_hw_cpu / m_num_package) * domain_index;
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
                throw geopm::Exception("XeonPlatformImp::read_signal: Invalid signal type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
    }

    void XeonPlatformImp::msr_initialize()
    {
        rapl_init();
        m_trigger_offset = m_msr_access->offset(M_TRIGGER_NAME);
        fixed_counters_init();
    }

    void XeonPlatformImp::init_telemetry(const TelemetryConfig &config)
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
            signame.push_back(*it);
            for (auto name_it = signame.begin(); name_it != signame.end(); ++name_it) {
                per_cpu_offsets.push_back(std::vector<off_t>(m_num_logical_cpu));
                auto signal_name = signal_to_msr_map.find(*name_it);
                if (signal_name == signal_to_msr_map.end()) {
                    throw Exception("XeonPlatformImp::init_telemetry(): Invalid signal string", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
                            throw Exception("XeonPlatformImp::init_telemetry(): Invalid MSR type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                        }
                        (*off_it) = (*lookup_it).second.offset;
                        ++cpu_idx;
                    }
                }
                auto msr_it = m_msr_signal_map_ptr->find(key);
                if (msr_it == m_msr_signal_map_ptr->end()) {
                    throw Exception("XeonPlatformImp::init_telemetry(): Invalid MSR type", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        m_control_msr_pair[M_RAPL_PKG_LIMIT] = std::make_pair(m_msr_access->offset("PKG_POWER_LIMIT"),
                                                              m_msr_access->write_mask("PKG_POWER_LIMIT"));
        m_control_msr_pair[M_RAPL_DRAM_LIMIT] = std::make_pair(m_msr_access->offset("DRAM_POWER_LIMIT"), 
                                                               m_msr_access->write_mask("DRAM_POWER_LIMIT"));
        m_control_msr_pair[M_IA32_PERF_CTL] = std::make_pair(m_msr_access->offset("IA32_PERF_CTL"),
                                                             m_msr_access->write_mask("IA32_PERF_CTL"));
    }

    double XeonPlatformImp::energy(void) const
    {
        double total_energy = 0;   
        for (int i = 0; i < m_num_package; ++i) {
            total_energy += msr_read(GEOPM_DOMAIN_PACKAGE, i, "PKG_ENERGY_STATUS");
            total_energy += msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_ENERGY_STATUS");
        }
        return total_energy;    
    }

    void XeonPlatformImp::msr_reset()
    {
        cbo_counters_reset();
        fixed_counters_reset();
    }

    void XeonPlatformImp::rapl_init()
    {
        uint64_t tmp;

        //Make sure units are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "RAPL_POWER_UNIT");
        m_power_units_inv = (double)(1 << (tmp & 0xF));
        m_energy_units = 1.0 / (double)(1 << ((tmp >> 8)  & 0x1F));
        if (m_dram_energy_units == 0.0) {
            m_dram_energy_units = m_energy_units;
        }
        double time_units = 1.0 / (double)(1 << ((tmp >> 16) & 0xF));

        for (int i = 1; i < m_num_package; i++) {
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "RAPL_POWER_UNIT");
            double power_inv = (double)(1 << (tmp & 0xF));
            double energy = 1.0 / (double)(1 << ((tmp >> 8)  & 0x1F));
            if (energy != m_energy_units || power_inv != m_power_units_inv) {
                throw Exception("detected inconsistent power units among packages", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        //Make sure bounds are consistent between packages
        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_INFO");
        m_tdp_pkg_watts = ((double)(tmp & 0x7fff)) / m_power_units_inv;
        m_min_pkg_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
        m_max_pkg_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "DRAM_POWER_INFO");
        m_min_dram_watts = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
        m_max_dram_watts = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;

        tmp = msr_read(GEOPM_DOMAIN_PACKAGE, 0, "PKG_POWER_LIMIT");
        // Set time window 1 to the minimum time window of 15 msec
        double tau = 0.015;
        uint64_t pkg_time_window_y = (uint64_t)std::log2(tau/time_units);
        uint64_t pkg_time_window_z = (uint64_t)(4.0 * ((tau / ((double)(1 << pkg_time_window_y) * time_units)) - 1.0));
        if ((pkg_time_window_z >> 2) != 0 || (pkg_time_window_y >> 5) != 0) {
            throw Exception("XeonPlatformImp::rapl_init(): Package time limit too large",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        double tau_inferred = (1 << pkg_time_window_y) * (1.0 + (pkg_time_window_z / 4.0)) * time_units;
        if ((tau - tau_inferred) > (tau  / 4.0)) {
            throw Exception("XeonPlatformImp::rapl_init(): Time window calculation inaccurate: "
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
                throw Exception("XeonPlatformImp::rapl_init(): Detected inconsistent power pkg bounds among packages",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            tmp = msr_read(GEOPM_DOMAIN_PACKAGE, i, "DRAM_POWER_INFO");
            double dram_min = ((double)((tmp >> 16) & 0x7fff)) / m_power_units_inv;
            double dram_max = ((double)((tmp >> 32) & 0x7fff)) / m_power_units_inv;
            if (dram_min != m_min_dram_watts || dram_max != m_max_dram_watts) {
                throw Exception("XeonPlatformImp::rapl_init(): Detected inconsistent power dram bounds among packages",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void XeonPlatformImp::cbo_counters_init(int counter_idx, uint32_t event)
    {
        for (int i = 0; i < m_num_hw_cpu; i++) {
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


    void XeonPlatformImp::fixed_counters_init()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR_CTRL", 0x0333);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_CTRL", 0x700000003);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_GLOBAL_OVF_CTRL", 0x0);
        }
    }

    void XeonPlatformImp::cbo_counters_reset()
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

    void XeonPlatformImp::fixed_counters_reset()
    {
        for (int cpu = 0; cpu < m_num_hw_cpu; cpu++) {
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR0", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR1", 0x0);
            msr_write(GEOPM_DOMAIN_CPU, cpu, "PERF_FIXED_CTR2", 0x0);
        }
    }

    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &snb_msr_signal_map(void)
    {
        static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> msr_signal_map({
            {"IA32_PERF_STATUS",        {0x0198, 0x0000000000000000, 32, 0, 8, 0x0ff, 0.1}},
            {"PKG_ENERGY_STATUS",       {0x0611, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"DRAM_ENERGY_STATUS",      {0x0619, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR0",         {0x0309, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR1",         {0x030A, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR2",         {0x030B, 0x0000000000000000, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR0",        {0x0D16, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR0",        {0x0D36, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR0",        {0x0D56, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR0",        {0x0D76, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR0",        {0x0D96, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR0",        {0x0DB6, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR0",        {0x0DD6, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR0",        {0x0DF6, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR0",        {0x0E16, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR0",        {0x0E36, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR0",       {0x0E56, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR0",       {0x0E76, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR0",       {0x0E96, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR0",       {0x0EB6, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR0",       {0x0ED6, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR1",        {0x0D17, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR1",        {0x0D37, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR1",        {0x0D57, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR1",        {0x0D77, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR1",        {0x0D97, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR1",        {0x0DB7, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR1",        {0x0DD7, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR1",        {0x0DF7, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR1",        {0x0E17, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR1",        {0x0E37, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR1",       {0x0E57, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR1",       {0x0E77, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR1",       {0x0E97, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR1",       {0x0EB7, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR1",       {0x0ED7, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}}
        });
        return msr_signal_map;
    }

    static const std::map<std::string, std::pair<off_t, uint64_t> > &snb_msr_control_map(void)
    {
        static const std::map<std::string, std::pair<off_t, uint64_t> > msr_control_map({
            {"IA32_PLATFORM_INFO",      {0x00CE, 0x0000000000000000}},
            {"IA32_PERF_CTL",           {0x0199, 0x000000010000ffff}},
            {"RAPL_POWER_UNIT",         {0x0606, 0x0000000000000000}},
            {"PKG_POWER_LIMIT",         {0x0610, 0x00ffffff00ffffff}},
            {"PKG_POWER_INFO",          {0x0614, 0x0000000000000000}},
            {"DRAM_POWER_LIMIT",        {0x0618, 0x0000000000ffffff}},
            {"DRAM_PERF_STATUS",        {0x061B, 0x0000000000000000}},
            {"DRAM_POWER_INFO",         {0x061C, 0x0000000000000000}},
            {"PERF_FIXED_CTR_CTRL",     {0x038D, 0x0000000000000bbb}},
            {"PERF_GLOBAL_CTRL",        {0x038F, 0x0000000700000003}},
            {"PERF_GLOBAL_OVF_CTRL",    {0x0390, 0xc000000700000003}},
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
            {"C14_MSR_PMON_CTL1",       {0x0ED1, 0x00000000ffffffff}}
        });
        return msr_control_map;
    }

    static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> &hsx_msr_signal_map(void)
    {
        static const std::map<std::string, struct IMSRAccess::m_msr_signal_entry> msr_signal_map({
            {"IA32_PERF_STATUS",        {0x0198, 0x0000000000000000, 32, 0, 8, 0x0ff, 0.1}},
            {"PKG_ENERGY_STATUS",       {0x0611, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"DRAM_ENERGY_STATUS",      {0x0619, 0x0000000000000000, 32, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR0",         {0x0309, 0xffffffffffffffff, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR1",         {0x030A, 0xffffffffffffffff, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"PERF_FIXED_CTR2",         {0x030B, 0xffffffffffffffff, 40, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR0",        {0x0E08, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR0",        {0x0E18, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR0",        {0x0E28, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR0",        {0x0E38, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR0",        {0x0E48, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR0",        {0x0E58, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR0",        {0x0E68, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR0",        {0x0E78, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR0",        {0x0E88, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR0",        {0x0E98, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR0",       {0x0EA8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR0",       {0x0EB8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR0",       {0x0EC8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR0",       {0x0ED8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR0",       {0x0EE8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C15_MSR_PMON_CTR0",       {0x0EF8, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C16_MSR_PMON_CTR0",       {0x0F08, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C17_MSR_PMON_CTR0",       {0x0F18, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C18_MSR_PMON_CTR0",       {0x0F28, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C19_MSR_PMON_CTR0",       {0x0F38, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C20_MSR_PMON_CTR0",       {0x0F48, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C21_MSR_PMON_CTR0",       {0x0F58, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C0_MSR_PMON_CTR1",        {0x0E09, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C1_MSR_PMON_CTR1",        {0x0E19, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C2_MSR_PMON_CTR1",        {0x0E29, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C3_MSR_PMON_CTR1",        {0x0E39, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C4_MSR_PMON_CTR1",        {0x0E49, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C5_MSR_PMON_CTR1",        {0x0E59, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C6_MSR_PMON_CTR1",        {0x0E69, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C7_MSR_PMON_CTR1",        {0x0E79, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C8_MSR_PMON_CTR1",        {0x0E89, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C9_MSR_PMON_CTR1",        {0x0E99, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C10_MSR_PMON_CTR1",       {0x0EA9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C11_MSR_PMON_CTR1",       {0x0EB9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C12_MSR_PMON_CTR1",       {0x0EC9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C13_MSR_PMON_CTR1",       {0x0ED9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C14_MSR_PMON_CTR1",       {0x0EE9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C15_MSR_PMON_CTR1",       {0x0EF9, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C16_MSR_PMON_CTR1",       {0x0F09, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C17_MSR_PMON_CTR1",       {0x0F19, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C18_MSR_PMON_CTR1",       {0x0F29, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C19_MSR_PMON_CTR1",       {0x0F39, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C20_MSR_PMON_CTR1",       {0x0F49, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}},
            {"C21_MSR_PMON_CTR1",       {0x0F59, 0x0000000000000000, 44, 0, 0, 0xffffffffffffffff, 1.0}}
        });
        return msr_signal_map;
    }

    static const std::map<std::string, std::pair<off_t, uint64_t> > &hsx_msr_control_map(void)
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
            {"C18_MSR_PMON_BOX_CTL",    {0x0F20, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_CTL",    {0x0F30, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_CTL",    {0x0F40, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_CTL",    {0x0F50, 0x00000000ffffffff}},
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
            {"C18_MSR_PMON_BOX_FILTER", {0x0F25, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_FILTER", {0x0F35, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_FILTER", {0x0F45, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_FILTER", {0x0F55, 0x00000000ffffffff}},
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
            {"C18_MSR_PMON_BOX_FILTER1",{0x0F26, 0x00000000ffffffff}},
            {"C19_MSR_PMON_BOX_FILTER1",{0x0F36, 0x00000000ffffffff}},
            {"C20_MSR_PMON_BOX_FILTER1",{0x0F46, 0x00000000ffffffff}},
            {"C21_MSR_PMON_BOX_FILTER1",{0x0F56, 0x00000000ffffffff}},
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
            {"C18_MSR_PMON_CTL0",       {0x0F21, 0x00000000ffffffff}},
            {"C19_MSR_PMON_CTL0",       {0x0F31, 0x00000000ffffffff}},
            {"C20_MSR_PMON_CTL0",       {0x0F41, 0x00000000ffffffff}},
            {"C21_MSR_PMON_CTL0",       {0x0F51, 0x00000000ffffffff}},
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
            {"C18_MSR_PMON_CTL1",       {0x0F22, 0x00000000ffffffff}},
            {"C19_MSR_PMON_CTL1",       {0x0F32, 0x00000000ffffffff}},
            {"C20_MSR_PMON_CTL1",       {0x0F42, 0x00000000ffffffff}},
            {"C21_MSR_PMON_CTL1",       {0x0F52, 0x00000000ffffffff}},
        });
        return msr_control_map;
    }
}
