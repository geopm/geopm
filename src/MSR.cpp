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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <numeric>

#include "MSR.hpp"
#include "MSRIO.hpp"
#include "PlatformTopology.hpp"
#include "Exception.hpp"
#include "geopm_sched.h"
#include "config.h"

#define  GEOPM_IOC_MSR_BATCH _IOWR('c', 0xA2, struct MSRIO::m_msr_batch_array)


namespace geopm
{
    /// @brief Class for translating between a double precision value
    /// and the encoded 64 bit MSR value.
    class MSREncode
    {
        public:
            MSREncode(const struct IMSR::m_encode_s &msre);
            MSREncode(int begin_bit, int end_bit, double scalar);
            virtual ~MSREncode();
            double decode(uint64_t field);
            uint64_t encode(double value);
            uint64_t mask(void);
        protected:
            int m_shift;
            uint64_t m_mask;
            double m_scalar;
            double m_inverse;
    };


    MSREncode::MSREncode(const struct IMSR::m_encode_s &msre)
        : MSREncode(msre.begin_bit, msre.end_bit, msre.scalar)
    {

    }

    MSREncode::MSREncode(int begin_bit, int end_bit, double scalar)
        : m_shift(begin_bit)
        , m_mask(((1ULL << (end_bit - begin_bit)) - 1) << begin_bit)
        , m_scalar(scalar)
        , m_inverse(1.0 / scalar)
    {

    }

    MSREncode::~MSREncode()
    {

    }

    double MSREncode::decode(uint64_t field)
    {
        return m_scalar * ((field & m_mask) >> m_shift);
    }

    uint64_t MSREncode::encode(double value)
    {
        return ((uint64_t)(value * m_inverse) << m_shift) & m_mask;
    }

    uint64_t MSREncode::mask(void)
    {
        return m_mask;
    }

    MSR::MSR(const std::string &msr_name,
             uint64_t offset,
             const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
             const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control)
        : m_name(msr_name)
        , m_offset(offset)
        , m_signal_encode(signal.size(), NULL)
        , m_control_encode(control.size())
        , m_domain_type(GEOPM_DOMAIN_CPU)
        , m_prog_msr(0)
        , m_prog_field_name(0)
        , m_prog_value(0)

    {
        init(signal, control);
    }

    MSR::MSR(const std::string &msr_name,
             const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
             const std::vector<const IMSR *> &prog_msr,
             const std::vector<std::string> &prog_field_name,
             const std::vector<double> &prog_value)
        : m_name(msr_name)
        , m_offset(0)
        , m_signal_encode(signal.size(), NULL)
        , m_control_encode(0)
        , m_domain_type(GEOPM_DOMAIN_CPU)
        , m_prog_msr(prog_msr)
        , m_prog_field_name(prog_field_name)
        , m_prog_value(prog_value)
    {
        const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > control;
        init(signal, control);
        if (m_prog_msr.size() != m_prog_field_name.size() ||
            m_prog_msr.size() != m_prog_value.size()) {
            throw Exception("MSR::MSR() input vectors for programming are not the same size",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }


    MSR::~MSR()
    {
        for (auto it = m_control_encode.rbegin(); it != m_control_encode.rend(); ++it) {
            delete (*it);
        }
        for (auto it = m_signal_encode.rbegin(); it != m_signal_encode.rend(); ++it) {
            delete (*it);
        }
    }

    void MSR::init(const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
                   const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control)
    {

        int idx = 0;
        for (auto it = signal.begin(); it != signal.end(); ++it, ++idx) {
            m_signal_map.insert(std::pair<std::string, int>((*it).first, idx));
            m_signal_encode[idx] = new MSREncode((*it).second);
        }
        idx = 0;
        for (auto it = control.begin(); it != control.end(); ++it, ++idx) {
            m_control_map.insert(std::pair<std::string, int>((*it).first, idx));
            m_control_encode[idx] = new MSREncode((*it).second);
        }

        if (m_name.compare(0, strlen("PKG_"), "PKG_") == 0) {
            m_domain_type = GEOPM_DOMAIN_PACKAGE;
        }
        else if (m_name.compare(0, strlen("DRAM_"), "DRAM_") == 0) {
            m_domain_type = GEOPM_DOMAIN_BOARD_MEMORY;
        }
    }

    void MSR::name(std::string &msr_name) const
    {
        msr_name = m_name;
    }

    void MSR::program(uint64_t offset,
                      int cpu_idx,
                      IMSRIO *msrio)
    {
        auto msr_it = m_prog_msr.begin();
        auto field_it = m_prog_field_name.begin();
        auto value_it = m_prog_value.begin();
        for (; msr_it != m_prog_msr.end(); ++msr_it, ++field_it, ++value_it) {
            int control_idx = (*msr_it)->control_index(*field_it);
            uint64_t field = 0, mask = 0;
            (*msr_it)->control(control_idx, *value_it, field, mask);
            msrio->write_msr(cpu_idx, (*msr_it)->offset(), mask, field);
        }
        m_offset = offset;
    }

    uint64_t MSR::offset(void) const
    {
        return m_offset;
    }

    int MSR::num_signal(void) const
    {
        return m_signal_encode.size();
    }

    int MSR::num_control(void) const
    {
        return m_control_encode.size();
    }

    void MSR::signal_name(int signal_idx,
                          std::string &name) const
    {
        if (signal_idx < 0 || signal_idx >= num_signal()) {
            throw Exception("MSR::signal_name(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        name = "";
        for (auto it = m_signal_map.begin(); it != m_signal_map.end(); ++it) {
            if ((*it).second == signal_idx) {
                name = (*it).first;
                break;
            }
        }
    }

    void MSR::control_name(int control_idx,
                           std::string &name) const
    {
        if (control_idx < 0 || control_idx >= num_control()) {
            throw Exception("MSR::control_name(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        name = "";
        for (auto it = m_control_map.begin(); it != m_control_map.end(); ++it) {
            if ((*it).second == control_idx) {
                name = (*it).first;
                break;
            }
        }
    }

    int MSR::signal_index(const std::string &name) const
    {
        int result = -1;
        auto it = m_signal_map.find(name);
        if (it != m_signal_map.end()) {
            result = (*it).second;
        }
        return result;
    }

    int MSR::control_index(const std::string &name) const
    {
        int result = -1;
        auto it = m_control_map.find(name);
        if (it != m_control_map.end()) {
            result = (*it).second;
        }
        return result;
    }

    double MSR::signal(int signal_idx,
                       uint64_t field) const
    {
        if (signal_idx < 0 || signal_idx >= num_signal()) {
            throw Exception("MSR::signal(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_encode[signal_idx]->decode(field);
    }

    void MSR::control(int control_idx,
                      double value,
                      uint64_t &field,
                      uint64_t &mask) const
    {
        if (control_idx < 0 || control_idx >= num_control()) {
            throw Exception("MSR::signal(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        field = m_control_encode[control_idx]->encode(value);
        mask = m_control_encode[control_idx]->mask();
    }

    int MSR::domain_type(void) const
    {
        return m_domain_type;
    }

    MSRSignal::MSRSignal(const IMSR *msr_obj,
                         int cpu_idx,
                         int signal_idx)
    {
        // Default constructor when no name is provided guesses the
        // signal name based on the MSR name and the field in that
        // MSR.  These are separated by a colon.
        std::ostringstream name;
        std::string msr_name;
        std::string msr_signal_name;
        msr_obj->name(msr_name);
        msr_obj->signal_name(signal_idx, msr_signal_name);
        name << msr_name << ":" << msr_signal_name;
        MSRSignal(msr_obj, cpu_idx, signal_idx, name.str());
    }

    MSRSignal::MSRSignal(const IMSR *msr_obj,
                         int cpu_idx,
                         int signal_idx,
                         const std::string &name)
        : MSRSignal(std::vector<IMSRSignal::m_signal_config_s>{IMSRSignal::m_signal_config_s{msr_obj, cpu_idx, signal_idx}}, name)
    {

    }

    MSRSignal::MSRSignal(const std::vector<IMSRSignal::m_signal_config_s> &config,
                         const std::string &name)
       : m_config(config)
       , m_name(name)
       , m_field_ptr(config.size(), NULL)
       , m_is_field_mapped(false)
    {

    }

    MSRSignal::~MSRSignal()
    {

    }

    void MSRSignal::name(std::string &signal_name) const
    {
        signal_name = m_name;
    }

    int MSRSignal::domain_type(void) const
    {
        throw Exception("MSRSignal::domain_type(): not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int MSRSignal::domain_idx(void) const
    {
        throw Exception("MSRSignal::domain_idx(): not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double MSRSignal::sample(void) const
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRControl::sample(): must call map() method before sample() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::vector<double> signal_vec(num_msr());
        auto signal_it = signal_vec.begin();
        auto field_it = m_field_ptr.begin();
        for (auto config_it = m_config.begin(); config_it != m_config.end(); ++config_it, ++signal_it, ++field_it) {
            *signal_it = (*config_it).msr_obj->signal((*config_it).signal_idx, *(*field_it));
        }
        return sample(signal_vec);
    }

    int MSRSignal::num_msr(void) const
    {
       return m_config.size();
    }

    void MSRSignal::offset(std::vector<uint64_t> &offset) const
    {
        offset.resize(m_config.size());
        size_t i = 0;
        for (auto &cc : m_config) {
            offset[i] = cc.msr_obj->offset();
            ++i;
        }
    }

    void MSRSignal::map_field(const uint64_t *field)
    {
        map_field({field});
    }

    void MSRSignal::map_field(const std::vector<const uint64_t *> &field)
    {
        if (field.size() != (size_t)num_msr()) {
            throw Exception("MSRSignal::map_field() field vector not properly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy(field.begin(), field.end(), m_field_ptr.begin());
        m_is_field_mapped = true;
    }

    double MSRSignal::sample(const std::vector<double> &per_msr_signal) const
    {
        return std::accumulate(per_msr_signal.begin(), per_msr_signal.end(), 0.0);
    }

    MSRControl::MSRControl(const IMSR *msr_obj,
                           int cpu_idx,
                           int control_idx)
    {
        // Default constructor when no name is provided guesses the
        // control name based on the MSR name and the field in that
        // MSR.  These are separated by a colon.
        std::ostringstream name;
        std::string msr_name;
        std::string msr_control_name;
        msr_obj->name(msr_name);
        msr_obj->control_name(control_idx, msr_control_name);
        name << msr_name << ":" << msr_control_name;
        MSRControl(msr_obj, cpu_idx, control_idx, name.str());
    }

    MSRControl::MSRControl(const IMSR *msr_obj,
                           int cpu_idx,
                           int control_idx,
                           const std::string &name)
        : MSRControl(std::vector<IMSRControl::m_control_config_s>{IMSRControl::m_control_config_s{msr_obj, cpu_idx, control_idx}}, name)
    {

    }

    MSRControl::MSRControl(const std::vector<struct IMSRControl::m_control_config_s> &config,
                           const std::string &name)
       : m_config(config)
       , m_name(name)
       , m_field_ptr(config.size(), NULL)
       , m_mask_ptr(config.size(), NULL)
       , m_is_field_mapped(false)
    {

    }

    MSRControl::~MSRControl()
    {

    }

    void MSRControl::name(std::string &name) const
    {
        name = m_name;
    }

    int MSRControl::domain_type(void) const
    {
        throw Exception("MSRControl::domain_type(): not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int MSRControl::domain_idx(void) const
    {
        throw Exception("MSRControl::domain_idx(): not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void MSRControl::adjust(double setting)
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRControl::adjust(): must call map() method before adjust() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto field_it = m_field_ptr.begin();
        auto mask_it = m_mask_ptr.begin();
        for (auto config_it = m_config.begin(); config_it != m_config.end(); ++config_it, ++field_it, ++mask_it) {
            (*config_it).msr_obj->control((*config_it).control_idx, setting, *(*field_it), *(*mask_it));
        }
    }

    int MSRControl::num_msr(void) const
    {
        return m_config.size();
    }

    void MSRControl::offset(std::vector<uint64_t> &offset) const
    {
        offset.resize(m_config.size());
        size_t i = 0;
        for (auto &cc : m_config) {
            offset[i] = cc.msr_obj->offset();
            ++i;
        }
    }

    void MSRControl::mask(std::vector<uint64_t> &mask) const
    {
        mask.resize(m_config.size());
        size_t i = 0;
        uint64_t field;
        for (auto &cc : m_config) {
            cc.msr_obj->control(cc.control_idx, 0.0, field, mask[i]);
            ++i;
        }
    }

    void MSRControl::map_field(uint64_t *field, uint64_t *mask)
    {
        map_field({field}, {mask});
    }

    void MSRControl::map_field(const std::vector<uint64_t *> &field, const std::vector<uint64_t *> &mask)
    {
        if (field.size() != (size_t)num_msr() ||
            mask.size() != (size_t)num_msr()) {
            throw Exception("MSRControl::map_field() field vector not properly sized",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy(field.begin(), field.end(), m_field_ptr.begin());
        std::copy(mask.begin(), mask.end(), m_mask_ptr.begin());
        m_is_field_mapped = true;
    }

}
