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

#include "geopm_sched.h"
#include "PlatformIO.hpp"
#include "PlatformIOInternal.hpp"
#include "PlatformTopology.hpp"

#include "MSR.hpp"
#include "MSRIO.hpp"
#include "Exception.hpp"

#include "config.h"

namespace geopm
{
    static const MSR *knl_msr(size_t &num_msr);
    static const MSR *hsx_msr(size_t &num_msr);
    static const MSR *snb_msr(size_t &num_msr);

    IPlatformIO &platform_io(void)
    {
        static PlatformIO instance;
        return instance;
    }

    void ctl_cpu_freq(std::vector<double> freq)
    {
        /// Temporary interface to adjust CPU frequency with the PlatformIO object
        static bool is_once = true;
        size_t num_cpu = geopm_sched_num_cpu();
        if (num_cpu != freq.size()) {
            throw Exception("geopm::ctl_cpu_freq(): input vector not properly sized to number of CPUs on system",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (is_once) {
            for (size_t cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
                platform_io().push_control("IA32_PERF_CTL:FREQ", GEOPM_DOMAIN_CPU, cpu_idx);
            }
            is_once = false;
        }
        platform_io().adjust(freq);
    }


    PlatformIO::PlatformIO()
        : m_num_cpu(geopm_sched_num_cpu())
        , m_is_active(false)
        , m_msrio(NULL)
    {
        init_msr();
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
            m_active_signal.push_back((*ncsm_it).second[cpu_idx]);
            IMSRSignal *msr_sig = dynamic_cast<IMSRSignal *>(m_active_signal.back());
            if (msr_sig) {
                std::vector<uint64_t> offset;
                msr_sig->offset(offset);
                for (int i = 0; i < msr_sig->num_msr(); ++i) {
                    m_msr_read_cpu_idx.push_back(cpu_idx);
                    m_msr_read_offset.push_back(offset[i]);
                }
            }
        }
        return result;
    }

    int PlatformIO::push_control(const std::string &control_name,
                                 int domain_type,
                                 int domain_idx)
    {
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
        auto ncsm_it = m_name_cpu_control_map.find(control_name);
        if (ncsm_it != m_name_cpu_control_map.end()) {
            result = m_active_control.size();
            m_active_control.push_back((*ncsm_it).second[cpu_idx]);
            IMSRControl *msr_ctl = dynamic_cast<IMSRControl *>(m_active_control.back());
            if (msr_ctl) {
                std::vector<uint64_t> offset;
                std::vector<uint64_t> mask;
                msr_ctl->offset(offset);
                msr_ctl->offset(mask);
                for (int i = 0; i < msr_ctl->num_msr(); ++i) {
                    m_msr_write_cpu_idx.push_back(cpu_idx);
                    m_msr_write_offset.push_back(offset[i]);
                    m_msr_write_mask.push_back(mask[i]);
                }
            }
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
        const MSR *msr_arr = NULL;
        size_t num_msr = 0;
        switch (cpuid()) {
            case M_CPUID_KNL:
                msr_arr = knl_msr(num_msr);
                break;
            case M_CPUID_HSX:
            case M_CPUID_BDX:
                msr_arr = hsx_msr(num_msr);
                break;
            case M_CPUID_SNB:
            case M_CPUID_IVT:
                msr_arr = snb_msr(num_msr);
                break;
            default:
                throw Exception("platform_io(): Unsupported CPUID",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::string name;
        for (const IMSR *msr_ptr = msr_arr;
             msr_ptr != msr_arr + num_msr;
             ++msr_ptr) {
            msr_ptr->name(name);
            m_name_msr_map.insert(std::pair<std::string, const IMSR *>(name, msr_ptr));
        }

        register_msr_control("IA32_PERF_CTL:FREQ");
        register_msr_control("IA32_PERF_CTL:ENABLE");
        /// @todo Fill in all other known MSR signals and controls.
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

    static const MSR *knl_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("IA32_PERF_CTL", 0x199,
                {},
                {{"FREQ", {0, 16, 1e-8}},
                 {"ENABLE", {32, 33, 1.0}}}),
                 /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }

    static const MSR *hsx_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("IA32_PERF_CTL", 0x199,
                {},
                {{"FREQ", {0, 16, 1e-8}},
                 {"ENABLE", {32, 33, 1.0}}}),
                 /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }

    static const MSR *snb_msr(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("IA32_PERF_CTL", 0x199,
                {},
                {{"FREQ", {0, 16, 1e-8}},
                 {"ENABLE", {32, 33, 1.0}}}),
                 /// @todo Define all the other MSRs.
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }
}
