/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <cpuid.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <utility>
#include <iomanip>
#include <sstream>

#include "geopm_sched.h"
#include "Exception.hpp"
#include "MSR.hpp"
#include "MSRIOGroup.hpp"
#include "MSRIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "config.h"

#define GEOPM_MSR_IO_GROUP_PLUGIN_NAME "MSR"

namespace geopm
{
    const MSR *msr_knl(size_t &num_msr);
    const MSR *msr_hsx(size_t &num_msr);
    const MSR *msr_snb(size_t &num_msr);
    static const MSR *init_msr_arr(int cpu_id, size_t &arr_size);

    MSRIOGroup::MSRIOGroup()
        : MSRIOGroup(std::unique_ptr<IMSRIO>(new MSRIO), cpuid(), geopm_sched_num_cpu())
    {

    }

    MSRIOGroup::MSRIOGroup(std::unique_ptr<IMSRIO> msrio, int cpuid, int num_cpu)
        : m_num_cpu(num_cpu)
        , m_is_active(false)
        , m_is_read(false)
        , m_msrio(std::move(msrio))
        , m_cpuid(cpuid)
        , m_name_prefix(plugin_name() + "::")
    {
        size_t num_msr = 0;
        const MSR *msr_arr = init_msr_arr(cpuid, num_msr);
        for (const MSR *msr_ptr = msr_arr;
             msr_ptr != msr_arr + num_msr;
             ++msr_ptr) {
            m_name_msr_map.insert(std::pair<std::string, const IMSR &>(msr_ptr->name(), *msr_ptr));
            for (int idx = 0; idx < msr_ptr->num_signal(); idx++) {
                register_msr_signal(m_name_prefix + msr_ptr->name() + ":" + msr_ptr->signal_name(idx));
            }
            for (int idx = 0; idx < msr_ptr->num_control(); idx++) {
                register_msr_control(m_name_prefix + msr_ptr->name() + ":" + msr_ptr->control_name(idx));
            }
        }

        register_msr_signal("FREQUENCY", "MSR::PERF_STATUS:FREQ");
        register_msr_control("FREQUENCY", "MSR::PERF_CTL:FREQ");

        register_msr_signal("ENERGY_PACKAGE", "MSR::PKG_ENERGY_STATUS:ENERGY");
        register_msr_signal("ENERGY_DRAM", "MSR::DRAM_ENERGY_STATUS:ENERGY");
        register_msr_signal("CYCLES_THREAD",    "MSR::PERF_FIXED_CTR1:CPU_CLK_UNHALTED_THREAD");
        register_msr_signal("CYCLES_REFERENCE", "MSR::PERF_FIXED_CTR2:CPU_CLK_UNHALTED_REF_TSC");

    }

    MSRIOGroup::~MSRIOGroup()
    {
        for (auto &ncsm : m_name_cpu_signal_map) {
            for (auto &sig_ptr : ncsm.second) {
                delete sig_ptr;
            }
        }
        for (auto &nccm : m_name_cpu_control_map) {
            for (auto &ctl_ptr : nccm.second) {
                delete ctl_ptr;
            }
        }

    }

    bool MSRIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_name_cpu_signal_map.find(signal_name) != m_name_cpu_signal_map.end();
    }

    bool MSRIOGroup::is_valid_control(const std::string &control_name) const
    {
        return m_name_cpu_control_map.find(control_name) != m_name_cpu_control_map.end();
    }

    int MSRIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = IPlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            /// @todo support for non-CPU domains.
            result = IPlatformTopo::M_DOMAIN_CPU;
        }
        return result;
    }

    int MSRIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = IPlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_control(control_name)) {
            /// @todo support for non-CPU domains.
            result = IPlatformTopo::M_DOMAIN_CPU;
        }
        return result;
    }

    int MSRIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        /// @todo support for non-CPU domains.
        if (domain_type != IPlatformTopo::M_DOMAIN_CPU) {
            throw Exception("MSRIOGroup::push_signal(): non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        if (m_is_active) {
            throw Exception("MSRIOGroup::push_signal(): cannot push a signal after read_batch() or adjust() has been called.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto ncsm_it = m_name_cpu_signal_map.find(signal_name);
        if (ncsm_it == m_name_cpu_signal_map.end()) {
            throw Exception("MSRIOGroup::push_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        /// @todo support for non-CPU domains.
        if (domain_idx < 0 || domain_idx > geopm_sched_num_cpu()) {
            throw Exception("MSRIOGroup::push_signal(): domain_idx out of bounds.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        // Check if signal was already pushed
        for (size_t ii = 0; !is_found && ii < m_active_signal.size(); ++ii) {
#ifdef GEOPM_DEBUG
            if (!m_active_signal[ii]) {
                throw Exception("MSRIOGroup::push_signal(): NULL MSRSignal pointer was saved in active signals",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            // signal_name may be alias, so use active signal MSR name
            std::string registered_name = ncsm_it->second[domain_idx]->name();
            /// @todo Support non-CPU domains and check domain type of the signal here
            if (m_active_signal[ii]->name() == registered_name &&
                m_active_signal[ii]->domain_idx() == domain_idx) {
                result = ii;
                is_found = true;
            }
        }

        /// @todo support for non-CPU domains.
        int cpu_idx = domain_idx;
        if (!is_found) {
            result = m_active_signal.size();
            m_active_signal.push_back(ncsm_it->second[cpu_idx]);
            MSRSignal *msr_sig = m_active_signal[result];
#ifdef GEOPM_DEBUG
            if (!msr_sig) {
                throw Exception("MSRIOGroup::push_signal(): NULL MSRSignal pointer was saved in active signals",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            uint64_t offset = msr_sig->offset();
            m_read_cpu_idx.push_back(cpu_idx);
            m_read_offset.push_back(offset);
        }
        return result;
    }

    int MSRIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        /// @todo support for non-CPU domains.
        if (domain_type != IPlatformTopo::M_DOMAIN_CPU) {
            throw Exception("MSRIOGroup::push_control(): non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        if (m_is_active) {
            throw Exception("MSRIOGroup::push_control(): cannot push a control after read_batch() or adjust() has been called.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        /// @todo support for non-CPU domains.
        if (domain_idx < 0 || domain_idx > geopm_sched_num_cpu()) {
            throw Exception("MSRIOGroup::push_control(): domain_idx out of bounds.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        auto nccm_it = m_name_cpu_control_map.find(control_name);
        if (nccm_it == m_name_cpu_control_map.end()) {
            throw Exception("MSRIOGroup::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int result = -1;
        bool is_found = false;
        // Check if control was already pushed
        for (size_t ii = 0; !is_found && ii < m_active_control.size(); ++ii) {
#ifdef GEOPM_DEBUG
            if (!m_active_control[ii]) {
                throw Exception("MSRIOGroup::push_control(): NULL MSRControl pointer was saved in active signals",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            // control_name may be alias, so use active control MSR name
            std::string registered_name = nccm_it->second[domain_idx]->name();
            /// @todo Support non-CPU domains and check domain type of the control here
            if (m_active_control[ii]->name() == registered_name &&
                m_active_control[ii]->domain_idx() == domain_idx) {
                result = ii;
                is_found = true;
            }
        }
        /// @todo support for non-CPU domains.
        int cpu_idx = domain_idx;
        if (!is_found) {
            result = m_active_control.size();
            m_is_adjusted.push_back(false);
            m_active_control.push_back(nccm_it->second[cpu_idx]);
            MSRControl *msr_ctl = m_active_control[result];
#ifdef GEOPM_DEBUG
            if (!msr_ctl) {
                throw Exception("MSRIOGroup::push_control(): NULL MSRControl pointer was saved in active controls",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
#endif
            uint64_t offset = msr_ctl->offset();
            uint64_t mask = msr_ctl->mask();
            m_write_cpu_idx.push_back(cpu_idx);
            m_write_offset.push_back(offset);
            m_write_mask.push_back(mask);
        }
        return result;
    }

    void MSRIOGroup::read_batch(void)
    {
        if (!m_is_active) {
            activate();
        }
        if (m_read_field.size()) {
            m_msrio->read_batch(m_read_field);
        }
        m_is_read = true;
    }

    void MSRIOGroup::write_batch(void)
    {
        if (m_active_control.size()) {
            if (std::any_of(m_is_adjusted.begin(), m_is_adjusted.end(), [](bool it) {return !it;})) {
                throw Exception("MSRIOGroup::write_batch() called before all controls were adjusted",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_msrio->write_batch(m_write_field);
        }
    }

    double MSRIOGroup::sample(int signal_idx) const
    {
        if (signal_idx < 0 || signal_idx >= (int)m_active_signal.size()) {
            throw Exception("MSRIOGroup::sample(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_read) {
            throw Exception("MSRIOGroup::sample() called before signal was read.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        return m_active_signal[signal_idx]->sample();
    }

    void MSRIOGroup::adjust(int control_idx, double setting)
    {
        if (control_idx < 0 || (unsigned)control_idx >= m_active_control.size()) {
            throw Exception("MSRIOGroup::adjust(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_active) {
            activate();
        }
        m_active_control[control_idx]->adjust(setting);
        m_is_adjusted[control_idx] = true;
    }

    double MSRIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx) const
    {
        /// @todo support for non-CPU domains.
        if (domain_type != IPlatformTopo::M_DOMAIN_CPU) {
            throw Exception("MSRIOGroup::read_signal(): non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        auto ncsm_it = m_name_cpu_signal_map.find(signal_name);
        if (ncsm_it == m_name_cpu_signal_map.end()) {
            throw Exception("MSRIOGroup::read_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        /// @todo support for non-CPU domains.
        if (domain_idx < 0 || domain_idx > geopm_sched_num_cpu()) {
            throw Exception("MSRIOGroup::read_signal(): domain_idx out of bounds.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int cpu_idx = domain_idx;
        MSRSignal signal = *(ncsm_it->second[cpu_idx]);
        uint64_t offset = signal.offset();
        uint64_t field = 0;
        signal.map_field(&field);
        field = m_msrio->read_msr(cpu_idx, offset);
        return signal.sample();
    }

    void MSRIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        /// @todo support for non-CPU domains.
        if (domain_type != IPlatformTopo::M_DOMAIN_CPU) {
            throw Exception("MSRIOGroup::write_control(): non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        auto nccm_it = m_name_cpu_control_map.find(control_name);
        if (nccm_it == m_name_cpu_control_map.end()) {
            throw Exception("MSRIOGroup::write_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        /// @todo support for non-CPU domains.
        if (domain_idx < 0 || domain_idx > geopm_sched_num_cpu()) {
            throw Exception("MSRIOGroup::write_control(): domain_idx out of bounds.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int cpu_idx = domain_idx;
        MSRControl control = *(nccm_it->second[cpu_idx]);
        uint64_t offset = control.offset();
        uint64_t field = 0;
        uint64_t mask = 0;
        control.map_field(&field, &mask);
        control.adjust(setting);
        m_msrio->write_msr(cpu_idx, offset, field, mask);
    }

    std::string MSRIOGroup::msr_whitelist(void) const
    {
        return msr_whitelist(m_cpuid);
    }

    std::string MSRIOGroup::msr_whitelist(int cpuid) const
    {
        size_t num_msr = 0;
        const MSR *msr_arr = init_msr_arr(cpuid, num_msr);
        std::ostringstream whitelist;
        whitelist << "# MSR        Write Mask           # Comment" << std::endl;
        whitelist << std::setfill('0') << std::hex;
        for (size_t idx = 0; idx < num_msr; idx++) {
            std::string msr_name = msr_arr[idx].name();
            uint64_t msr_offset = msr_arr[idx].offset();
            size_t num_signals = msr_arr[idx].num_signal();
            size_t num_controls = msr_arr[idx].num_control();
            uint64_t write_mask = 0;

            if (!num_signals && !num_controls) {
                throw Exception("MSRIOGroup::msr_whitelist(): invalid msr",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            if (num_controls) {
                for (size_t cidx = 0; cidx < num_controls; cidx++) {
                    uint64_t idx_field = 0, idx_mask = 0;
                    msr_arr[idx].control(cidx, 1, idx_field, idx_mask);
                    write_mask |= idx_mask;
                }
            }
            whitelist << "0x" << std::setw(8) << msr_offset << "   0x" << std::setw(16) << write_mask << "   # \"" << msr_name << "\"" << std::endl;
        }
        return whitelist.str();
    }

    int MSRIOGroup::cpuid(void) const
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
        ext_family = (proc_info & extended_family_mask) >> 20;

        if (family == 6) {
            model+=(ext_model << 4);
        }
        else if (family == 15) {
            model+=(ext_model << 4);
            family+=ext_family;
        }

        return ((family << 8) + model);
    }

    void MSRIOGroup::activate(void)
    {
        m_msrio->config_batch(m_read_cpu_idx, m_read_offset,
                              m_write_cpu_idx, m_write_offset, m_write_mask);
        m_read_field.resize(m_read_cpu_idx.size());
        m_write_field.resize(m_write_cpu_idx.size());
        size_t msr_idx = 0;
        for (auto &msr_sig : m_active_signal) {
            const uint64_t *field_ptr = &(m_read_field[msr_idx]);
            msr_sig->map_field(field_ptr);
            ++msr_idx;
        }
        msr_idx = 0;
        for (auto &msr_ctl : m_active_control) {
            uint64_t *field_ptr = &(m_write_field[msr_idx]);
            uint64_t *mask_ptr = &(m_write_mask[msr_idx]);
            msr_ctl->map_field(field_ptr, mask_ptr);
            ++msr_idx;
        }
        m_is_active = true;
    }

    void MSRIOGroup::register_msr_signal(const std::string &msr_name)
    {
        register_msr_signal(msr_name, msr_name);
    }

    void MSRIOGroup::register_msr_signal(const std::string &signal_name, const std::string &msr_name_field)
    {
        Exception ex("MSRIOGroup::register_msr_signal(): msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"",
                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        if (msr_name_field.compare(0, m_name_prefix.size(), m_name_prefix) != 0) {
            throw ex;
        }
        std::string name_field = msr_name_field.substr(m_name_prefix.size());
        size_t colon_pos = name_field.find(':');
        if (colon_pos == std::string::npos) {
            throw ex;
        }
        std::string msr_name(name_field.substr(0, colon_pos));
        std::string field_name(name_field.substr(colon_pos + 1));

        // Insert the signal name with an empty vector into the map
        auto ins_ret = m_name_cpu_signal_map.insert(std::pair<std::string, std::vector<MSRSignal *> >(signal_name, {}));
        // Get reference to the per-cpu signal vector
        std::vector <MSRSignal *> &cpu_signal = (*(ins_ret.first)).second;
        // Check to see if the signal name has already been registered
        if (!ins_ret.second) {
            throw Exception("MSRIOGroup::register_msr_signal(): signal_name " + signal_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        cpu_signal.resize(m_num_cpu, nullptr);

        auto name_msr_it = m_name_msr_map.find(msr_name);
        if (name_msr_it == m_name_msr_map.end()) {
            throw Exception("MSRIOGroup::register_msr_signal(): msr_name could not be found: " + msr_name,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const IMSR &msr_obj = name_msr_it->second;
        int signal_idx = msr_obj.signal_index(field_name);
        if (signal_idx == -1) {
            throw Exception("MSRIOGroup::register_msr_signal(): field_name could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            cpu_signal[cpu_idx] = new MSRSignal(msr_obj, PlatformTopo::M_DOMAIN_CPU,
                                                cpu_idx, signal_idx);
        }
    }

    void MSRIOGroup::register_msr_control(const std::string &control_name)
    {
        register_msr_control(control_name, control_name);
    }

    void MSRIOGroup::register_msr_control(const std::string &control_name, const std::string &msr_name_field)
    {
        Exception ex("MSRIOGroup::register_msr_control(): msr_name_field must be of the form \"MSR::<msr_name>:<field_name>\"",
                     GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        if (msr_name_field.compare(0, m_name_prefix.size(), m_name_prefix) != 0) {
            throw ex;
        }
        std::string name_field = msr_name_field.substr(m_name_prefix.size());
        size_t colon_pos = name_field.find(':');
        if (colon_pos == std::string::npos) {
            throw ex;
        }
        std::string msr_name(name_field.substr(0, colon_pos));
        std::string field_name(name_field.substr(colon_pos + 1));

        // Insert the control name with an empty vector into the map
        auto ins_ret = m_name_cpu_control_map.insert(std::pair<std::string, std::vector<MSRControl *> >(control_name, {}));
        // Get reference to the per-cpu control vector
        std::vector <MSRControl *> &cpu_control = (*(ins_ret.first)).second;
        // Check to see if the control name has already been registered
        if (!ins_ret.second) {
            throw Exception("MSRIOGroup::register_msr_control(): control_name " + control_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);

        }
        cpu_control.resize(m_num_cpu, NULL);

        auto name_msr_it = m_name_msr_map.find(msr_name);
        if (name_msr_it == m_name_msr_map.end()) {
            throw Exception("MSRIOGroup::register_msr_control(): msr_name could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const IMSR &msr_obj = name_msr_it->second;
        int control_idx = msr_obj.control_index(field_name);
        if (control_idx == -1) {
            throw Exception("MSRIOGroup::register_msr_control(): field_name could not be found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            cpu_control[cpu_idx] = new MSRControl(msr_obj, PlatformTopo::M_DOMAIN_CPU, cpu_idx, control_idx);
        }
    }

    std::string MSRIOGroup::plugin_name(void)
    {
        return GEOPM_MSR_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> MSRIOGroup::make_plugin(void)
    {
        return geopm::make_unique<MSRIOGroup>();
    }

    const MSR *init_msr_arr(int cpu_id, size_t &arr_size)
    {
        const MSR *msr_arr = NULL;
        arr_size = 0;
        switch (cpu_id) {
            case MSRIOGroup::M_CPUID_KNL:
                msr_arr = msr_knl(arr_size);
                break;
            case MSRIOGroup::M_CPUID_HSX:
            case MSRIOGroup::M_CPUID_BDX:
                msr_arr = msr_hsx(arr_size);
                break;
            case MSRIOGroup::M_CPUID_SNB:
            case MSRIOGroup::M_CPUID_IVT:
                msr_arr = msr_snb(arr_size);
                break;
            default:
                throw Exception("MSRIOGroup: Unsupported CPUID",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return msr_arr;
    }
}
