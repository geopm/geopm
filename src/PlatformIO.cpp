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

#include <cpuid.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "geopm_sched.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopology.hpp"
#include "TimeSignal.hpp"

#include "MSR.hpp"
#include "MSRIO.hpp"
#include "Exception.hpp"

#include "config.h"

namespace geopm
{
    static const MSR *knl_msr(size_t &num_msr);
    static const MSR *hsx_msr(size_t &num_msr);
    static const MSR *snb_msr(size_t &num_msr);
    static const MSR *get_msr_arr(int cpu_id, size_t &num_msr);

    IPlatformIO &platform_io(void)
    {
        static PlatformIO instance;
        return instance;
    }

    PlatformIO::PlatformIO()
        : m_num_cpu(geopm_sched_num_cpu())
        , m_is_init(false)
        , m_is_active(false)
        , m_msrio(NULL)
    {

    }

    PlatformIO::~PlatformIO()
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
        delete m_msrio;
    }

    int PlatformIO::push_signal(const std::string &signal_name,
                                int domain_type,
                                int domain_idx)
    {
        if (!m_is_init) {
            init();
        }
        if (m_is_active) {
            throw Exception("PlatformIO::push_signal(): clear() has not been called since last call to sample().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        /// @todo support for non-CPU domains.
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("PlatformIO: non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        int cpu_idx = domain_idx;
        auto ncsm_it = m_name_cpu_signal_map.find(signal_name);
        if (ncsm_it != m_name_cpu_signal_map.end()) {
            result = m_active_signal.size();
            if ((*ncsm_it).second.size() == 1) {
                m_active_signal.push_back((*ncsm_it).second[0]);
            }
            else {
                m_active_signal.push_back((*ncsm_it).second[cpu_idx]);
            }
            IMSRSignal *msr_sig = dynamic_cast<IMSRSignal *>(m_active_signal.back());
            if (msr_sig) {
                std::vector<uint64_t> offset;
                msr_sig->offset(offset);
                m_msr_read_signal_off.push_back(m_msr_read_cpu_idx.size());
                m_msr_read_signal_len.push_back(msr_sig->num_msr());
                for (int i = 0; i < msr_sig->num_msr(); ++i) {
                    m_msr_read_cpu_idx.push_back(cpu_idx);
                    m_msr_read_offset.push_back(offset[i]);
                }
            }
        }
        else {
            std::ostringstream err_str;
            err_str << "PlatformIO::push_signal(): signal name \""
                    << signal_name << "\" not found";
            throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    int PlatformIO::push_control(const std::string &control_name,
                                 int domain_type,
                                 int domain_idx)
    {
        if (!m_is_init) {
            init();
        }
        if (m_is_active) {
            throw Exception("PlatformIO::push_control(): clear() has not been called since last call to sample().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        /// @todo support for non-CPU domains.
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("PlatformIO: non-CPU domain_type not implemented.",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        int cpu_idx = domain_idx;
        auto nccm_it = m_name_cpu_control_map.find(control_name);
        if (nccm_it != m_name_cpu_control_map.end()) {
            result = m_active_control.size();
            m_active_control.push_back((*nccm_it).second[cpu_idx]);
            IMSRControl *msr_ctl = dynamic_cast<IMSRControl *>(m_active_control.back());
            if (msr_ctl) {
                std::vector<uint64_t> offset;
                std::vector<uint64_t> mask;
                msr_ctl->offset(offset);
                msr_ctl->mask(mask);
                m_msr_write_control_off.push_back(m_msr_write_cpu_idx.size());
                m_msr_write_control_len.push_back(msr_ctl->num_msr());
                for (int i = 0; i < msr_ctl->num_msr(); ++i) {
                    m_msr_write_cpu_idx.push_back(cpu_idx);
                    m_msr_write_offset.push_back(offset[i]);
                    m_msr_write_mask.push_back(mask[i]);
                }
            }
        }
        else {
            std::ostringstream err_str;
            err_str << "PlatformIO::push_control(): control name \""
                    << control_name << "\" not found";
            throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void PlatformIO::clear(void)
    {
        throw Exception("PlatformIO: class not implemented.",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        m_msr_read_cpu_idx.resize(0);
        m_msr_read_cpu_idx.resize(0);
        m_msr_read_offset.resize(0);
        m_msr_write_cpu_idx.resize(0);
        m_msr_write_offset.resize(0);
        m_msr_write_mask.resize(0);
        m_is_active = false;
    }

    void PlatformIO::init(void)
    {
        init_time();
        init_msr();
        m_is_init = true;
    }

    void PlatformIO::activate(void)
    {
        activate_msr();
        m_is_active = true;
    }

    void PlatformIO::activate_msr(void)
    {
        if (!m_msrio) {
            m_msrio = new MSRIO;
        }
        m_msrio->config_batch(m_msr_read_cpu_idx, m_msr_read_offset,
                              m_msr_write_cpu_idx, m_msr_write_offset, m_msr_write_mask);
        m_msr_read_field.resize(m_msr_read_cpu_idx.size());
        m_msr_write_field.resize(m_msr_write_cpu_idx.size());
        size_t msr_idx = 0;
        for (auto &sig : m_active_signal) {
            IMSRSignal *msr_sig = dynamic_cast<IMSRSignal *>(sig);
            if (msr_sig) {
                std::vector<const uint64_t *> field_ptr(msr_sig->num_msr());
                for (auto &fp : field_ptr) {
                    fp = m_msr_read_field.data() + msr_idx;
                    ++msr_idx;
                }
                msr_sig->map_field(field_ptr);
            }
        }
        msr_idx = 0;
        for (auto &ctl : m_active_control) {
            IMSRControl *msr_ctl = dynamic_cast<IMSRControl *>(ctl);
            if (msr_ctl) {
                std::vector<uint64_t *> field_ptr(msr_ctl->num_msr());
                std::vector<uint64_t *> mask_ptr(msr_ctl->num_msr());
                size_t msr_idx_save = msr_idx;
                for (auto &fp : field_ptr) {
                    fp = m_msr_write_field.data() + msr_idx;
                    ++msr_idx;
                }
                msr_idx = msr_idx_save;
                for (auto &mp : mask_ptr) {
                    mp = m_msr_write_mask.data() + msr_idx;
                    ++msr_idx;
                }
                msr_ctl->map_field(field_ptr, mask_ptr);
            }
        }
    }

    double PlatformIO::sample(int signal_idx)
    {
        if (signal_idx < 0 || (unsigned)signal_idx >= m_active_signal.size()) {
            throw Exception("PlatformIO::sample() signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_active) {
            activate();
        }

        auto field_it = m_msr_read_field.begin() + m_msr_read_signal_off[signal_idx];
        auto cpu_it = m_msr_read_cpu_idx.begin() + m_msr_read_signal_off[signal_idx];
        auto off_it = m_msr_read_offset.begin() + m_msr_read_signal_off[signal_idx];
        for (int i = 0; i < m_msr_read_signal_len[signal_idx]; ++i) {
            *field_it = m_msrio->read_msr(*cpu_it, *off_it);
            ++field_it;
            ++cpu_it;
            ++off_it;
        }
        return m_active_signal[signal_idx]->sample();
    }

    void PlatformIO::adjust(int control_idx,
                            double setting)
    {
        if (control_idx < 0 || (unsigned)control_idx >= m_active_control.size()) {
            throw Exception("PlatformIO::adjust() control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!m_is_active) {
            activate();
        }
        m_active_control[control_idx]->adjust(setting);

        auto field_it = m_msr_write_field.begin() + m_msr_write_control_off[control_idx];
        auto cpu_it = m_msr_write_cpu_idx.begin() + m_msr_write_control_off[control_idx];
        auto off_it = m_msr_write_offset.begin() + m_msr_write_control_off[control_idx];
        auto mask_it = m_msr_write_mask.begin() + m_msr_write_control_off[control_idx];
        for (int i = 0; i < m_msr_write_control_len[control_idx]; ++i) {
            m_msrio->write_msr(*cpu_it, *off_it, *mask_it, *field_it);
            ++field_it;
            ++cpu_it;
            ++off_it;
            ++mask_it;
        }
    }

    void PlatformIO::sample(std::vector<double> &signal)
    {
        if (!m_is_active) {
            activate();
        }
        m_msrio->read_batch(m_msr_read_field);
        signal.resize(m_active_signal.size());
        auto sig_it = signal.begin();
        for (auto &as : m_active_signal) {
            *sig_it = as->sample();
            ++sig_it;
        }
    }

    void PlatformIO::adjust(const std::vector<double> &setting)
    {
        if (!m_is_active) {
            activate();
        }
        if (setting.size() != m_active_control.size()) {
            throw Exception("PlatformIO::adjust() setting vector improperly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto set_it = setting.begin();
        for (auto &ac : m_active_control) {
            ac->adjust(*set_it);
            ++set_it;
        }
        m_msrio->write_batch(m_msr_write_field);
    }

    void PlatformIO::init_msr(void)
    {
        size_t num_msr = 0;
        const MSR *msr_arr = get_msr_arr(cpuid(), num_msr);
        for (const MSR *msr_ptr = msr_arr;
             msr_ptr != msr_arr + num_msr;
             ++msr_ptr) {
            m_name_msr_map.insert(std::pair<std::string, const IMSR *>(msr_ptr->name(), msr_ptr));
            for (int idx = 0; idx < msr_ptr->num_signal(); idx++) {
                register_msr_signal(msr_ptr->name() + ":" + msr_ptr->signal_name(idx));
            }
            for (int idx = 0; idx < msr_ptr->num_control(); idx++) {
                register_msr_control(msr_ptr->name() + ":" + msr_ptr->control_name(idx));
            }
        }
    }

    void PlatformIO::init_time(void)
    {
        // Insert the signal name with an empty vector into the map
        auto ins_ret = m_name_cpu_signal_map.insert(std::pair<std::string, std::vector<ISignal *> >("TIME", {}));
        // Get reference to the per-cpu signal vector
        std::vector <ISignal *> &signal = (*(ins_ret.first)).second;
        // Check to see if the signal name has already been registered
        if (!ins_ret.second) {
            if (signal.size() != 1 ||
                dynamic_cast<TimeSignal *>(signal[0]) == NULL) {
                throw Exception("PlatformIO::init_time() class other than TimeSignal registered as the TIME signal.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
        else {
            signal = {new TimeSignal()};
        }
    }

    void PlatformIO::register_msr_signal(const std::string &signal_name)
    {
        size_t colon_pos = signal_name.find(':');
        if (colon_pos == std::string::npos) {
            throw Exception("PlatformIO::register_msr_signal(): signal_name must be of the form \"msr_name:field_name\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<std::string> msr_name_vec({signal_name.substr(0, colon_pos)});
        std::vector<std::string> field_name_vec({signal_name.substr(colon_pos + 1)});
        register_msr_signal(signal_name, msr_name_vec, field_name_vec);
    }


    void PlatformIO::register_msr_signal(const std::string &signal_name,
                                         const std::vector<std::string> &msr_name,
                                         const std::vector<std::string> &field_name)
    {
        // Assert that msr_name and field_name are the same size.
        if (msr_name.size() != field_name.size()) {
            throw Exception("PlatformIO::register_msr_signal(): signal_name vector length does not match msr_name",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Insert the signal name with an empty vector into the map
        auto ins_ret = m_name_cpu_signal_map.insert(std::pair<std::string, std::vector<ISignal *> >(signal_name, {}));
        // Get reference to the per-cpu signal vector
        std::vector <ISignal *> &cpu_signal = (*(ins_ret.first)).second;
        // Check to see if the signal name has already been registered
        if (!ins_ret.second) {
            /* delete previous signals */
            for (auto &cs : cpu_signal) {
                delete cs;
            }
        }
        cpu_signal.resize(m_num_cpu, NULL);
        int num_field = field_name.size();

        std::vector<struct IMSRSignal::m_signal_config_s> signal_config(num_field);
        int field_idx = 0;
        for (auto &sc : signal_config) {
            auto name_msr_it = m_name_msr_map.find(msr_name[field_idx]);
            if (name_msr_it == m_name_msr_map.end()) {
                throw Exception("PlatformIO::register_msr_signal(): msr_name could not be found",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            sc.msr_obj = (*name_msr_it).second;
            sc.signal_idx = sc.msr_obj->signal_index(field_name[field_idx]);
            if (sc.signal_idx == -1) {
                throw Exception("PlatformIO::register_msr_signal(): field_name could not be found",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            ++field_idx;
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            for (auto &sc : signal_config) {
                sc.cpu_idx = cpu_idx;
            }
            cpu_signal[cpu_idx] = new MSRSignal(signal_config, signal_name);
        }
    }

    void PlatformIO::register_msr_control(const std::string &control_name)
    {
        size_t colon_pos = control_name.find(':');
        if (colon_pos == std::string::npos) {
            throw Exception("PlatformIO::register_msr_control(): control_name must be of the form \"msr_name:field_name\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<std::string> msr_name_vec({control_name.substr(0, colon_pos)});
        std::vector<std::string> field_name_vec({control_name.substr(colon_pos + 1)});
        register_msr_control(control_name, msr_name_vec, field_name_vec);
    }

    void PlatformIO::register_msr_control(const std::string &control_name,
                                          const std::vector<std::string> &msr_name,
                                          const std::vector<std::string> &field_name)
    {
        // Assert that msr_name and field_name are the same size.
        if (msr_name.size() != field_name.size()) {
            throw Exception("PlatformIO::register_msr_control(): control_name vector length does not match msr_name",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // Insert the control name with an empty vector into the map
        auto ins_ret = m_name_cpu_control_map.insert(std::pair<std::string, std::vector<IControl *> >(control_name, {}));
        // Get reference to the per-cpu control vector
        std::vector <IControl *> &cpu_control = (*(ins_ret.first)).second;
        // Check to see if the control name has already been registered
        if (!ins_ret.second) {
            /* delete previous controls */
            for (auto &cc : cpu_control) {
                delete cc;
            }
        }
        cpu_control.resize(m_num_cpu, NULL);
        int num_field = field_name.size();

        std::vector<struct IMSRControl::m_control_config_s> control_config(num_field);
        int field_idx = 0;
        for (auto &sc : control_config) {
            auto name_msr_it = m_name_msr_map.find(msr_name[field_idx]);
            if (name_msr_it == m_name_msr_map.end()) {
                throw Exception("PlatformIO::register_msr_control(): msr_name could not be found",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            sc.msr_obj = (*name_msr_it).second;
            sc.control_idx = sc.msr_obj->control_index(field_name[field_idx]);
            if (sc.control_idx == -1) {
                throw Exception("PlatformIO::register_msr_control(): field_name could not be found",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            ++field_idx;
        }

        for (int cpu_idx = 0; cpu_idx < m_num_cpu; ++cpu_idx) {
            for (auto &sc : control_config) {
                sc.cpu_idx = cpu_idx;
            }
            cpu_control[cpu_idx] = new MSRControl(control_config, control_name);
        }
    }

    int PlatformIO::cpuid(void)
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

    std::string PlatformIO::log(int signal_idx, double sample)
    {
        throw Exception("PlatformIO::log(): Not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    std::string PlatformIO::msr_whitelist(void)
    {
        return msr_whitelist(cpuid());
    }

    std::string PlatformIO::msr_whitelist(int cpuid)
    {
        size_t num_msr = 0;
        const MSR *msr_arr = get_msr_arr(cpuid, num_msr);
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
                std::ostringstream err_str;
                err_str << "PlatformIO::msr_whitelist(): invalid msr";
                throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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

    static const MSR *knl_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("PERF_STATUS", 0x198,
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}},
                {}),
            MSR("PERF_CTL", 0x199,
                {},
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PKG_RAPL_UNIT", 0x606,
                {{"POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 4,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 8.0}}, // Signal is 1.0 because the units should be 0.125 Watts
                 {"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 13,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 1.6384e4}}, // Signal is 1.0 because the units should be 6.103515625e-05 Joules.
                 {"TIME", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 20,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 1.024e3}}}, // Signal is 1.0 because the units should be 9.765625e-04 seconds.
                {}),
            MSR("PKG_POWER_LIMIT", 0x610,
                {},
                {{"SOFT_POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"SOFT_LIMIT_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 15,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"SOFT_CLAMP_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 17,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"SOFT_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 17,
                      .end_bit   = 24,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}},
                 {"HARD_POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"HARD_LIMIT_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 47,
                      .end_bit   = 48,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"HARD_CLAMP_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 49,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"HARD_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 49,
                      .end_bit   = 56,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}}),
            MSR("PKG_ENERGY_STATUS", 0x611,
                {{"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 6.103515625e-05}}},
                {}),
            MSR("PKG_POWER_INFO", 0x614,
                {{"THERMAL_SPEC_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MIN_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 31,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 55,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}},
                {}),
            MSR("DRAM_POWER_LIMIT", 0x618,
                {},
                {{"POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 15,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 17,
                      .end_bit   = 24,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}}),
            MSR("DRAM_ENERGY_STATUS", 0x619,
                {{"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 6.103515625e-05}}},
                {}),
            MSR("DRAM_PERF_STATUS", 0x61B,
                {{"THROTTLE_TIME", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}},
                {}),
            MSR("DRAM_POWER_INFO", 0x61C,
                {{"THERMAL_SPEC_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MIN_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 31,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_TIME_WINDOW", (struct IMSR::m_encode_s) { // Not correctly documented in SDM, this description is correct.
                      .begin_bit = 48,
                      .end_bit   = 55,
                      .domain    = IPlatformIO::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}},
                {}),
            MSR("PERF_FIXED_CTR_CTRL", 0x38D,
                {},
                {{"EN0_OS", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 1,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN0_USR", (struct IMSR::m_encode_s) {
                      .begin_bit = 1,
                      .end_bit   = 2,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN0_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 3,
                      .end_bit   = 4,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_OS", (struct IMSR::m_encode_s) {
                      .begin_bit = 4,
                      .end_bit   = 5,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_USR", (struct IMSR::m_encode_s) {
                      .begin_bit = 5,
                      .end_bit   = 6,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 7,
                      .end_bit   = 8,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_OS", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 9,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_USR", (struct IMSR::m_encode_s) {
                      .begin_bit = 9,
                      .end_bit   = 10,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 11,
                      .end_bit   = 12,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PERF_GLOBAL_CTRL", 0x38F,
                {},
                {{"EN_PMC0", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 1,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_PMC1", (struct IMSR::m_encode_s) {
                      .begin_bit = 1,
                      .end_bit   = 2,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR0", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR1", (struct IMSR::m_encode_s) {
                      .begin_bit = 33,
                      .end_bit   = 34,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR2", (struct IMSR::m_encode_s) {
                      .begin_bit = 34,
                      .end_bit   = 35,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PERF_GLOBAL_OVF_CTRL", 0x390,
                {},
                {{"CLEAR_OVF_PMC0", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 1,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_PMC1", (struct IMSR::m_encode_s) {
                      .begin_bit = 1,
                      .end_bit   = 2,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR0", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR1", (struct IMSR::m_encode_s) {
                      .begin_bit = 33,
                      .end_bit   = 34,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR2", (struct IMSR::m_encode_s) {
                      .begin_bit = 34,
                      .end_bit   = 35,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PERF_FIXED_CTR0", 0x309,
                {{"INST_RETIRED_ANY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR1", 0x30A,
                {{"CPU_CLK_UNHALTED_THREAD", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR2", 0x30B,
                {{"CPU_CLK_UNHALTED_REF_TSC", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformIO::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }

    static const MSR *hsx_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("PERF_STATUS", 0x198,
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}},
                {}),
            MSR("PERF_CTL", 0x199,
                {},
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
                 /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }

    static const MSR *snb_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("PERF_STATUS", 0x198,
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}},
                {}),
            MSR("PERF_CTL", 0x199,
                {},
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformIO::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
                 /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }

    static const MSR *get_msr_arr(int cpu_id, size_t &num_msr)
    {
        const MSR *msr_arr = NULL;
        num_msr = 0;
        switch (cpu_id) {
            case PlatformIO::M_CPUID_KNL:
                msr_arr = knl_msr(num_msr);
                break;
            case PlatformIO::M_CPUID_HSX:
            case PlatformIO::M_CPUID_BDX:
                msr_arr = hsx_msr(num_msr);
                break;
            case PlatformIO::M_CPUID_SNB:
            case PlatformIO::M_CPUID_IVT:
                msr_arr = snb_msr(num_msr);
                break;
            default:
                throw Exception("platform_io(): Unsupported CPUID",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return msr_arr;
    }
}
