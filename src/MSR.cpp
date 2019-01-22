/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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
#include <string.h>
#include <cfloat>
#include <cmath>
#include <sstream>
#include <numeric>

#include "MSR.hpp"
#include "MSRIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "geopm_sched.h"
#include "geopm_hash.h"
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
            MSREncode(int begin_bit, int end_bit, int function, double scalar);
            virtual ~MSREncode() = default;
            double decode(uint64_t field, uint64_t &last_field, uint64_t &num_overflow);
            uint64_t encode(double value);
            uint64_t mask(void);
            int decode_function(void);
        private:
            const int m_function;
            int m_shift;
            int m_num_bit;
            uint64_t m_mask;
            uint64_t m_subfield_max;
            double m_scalar;
            double m_inverse;
    };


    MSREncode::MSREncode(const struct IMSR::m_encode_s &msre)
        : MSREncode(msre.begin_bit, msre.end_bit, msre.function, msre.scalar)
    {

    }

    MSREncode::MSREncode(int begin_bit, int end_bit, int function, double scalar)
        : m_function(function)
        , m_shift(begin_bit)
        , m_num_bit(end_bit - begin_bit)
        , m_mask(((1ULL << (end_bit - begin_bit)) - 1) << begin_bit)
        , m_subfield_max((1ULL << m_num_bit) - 1)
        , m_scalar(scalar)
        , m_inverse(1.0 / scalar)
    {
#ifdef GEOPM_DEBUG
        if (m_num_bit >= 64) {
            throw Exception("MSREncode: 64 bit fields are not supported.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    double MSREncode::decode(uint64_t field, uint64_t &last_field, uint64_t &num_overflow)
    {
        double result = NAN;
        uint64_t subfield = (field & m_mask) >> m_shift;
        uint64_t subfield_last = (last_field & m_mask) >> m_shift;
        uint64_t float_y, float_z;
        switch (m_function) {
            case IMSR::M_FUNCTION_LOG_HALF:
                // F = S * 2.0 ^ -X
                result = 1.0 / (1ULL << subfield);
                break;
            case IMSR::M_FUNCTION_7_BIT_FLOAT:
                // F = S * 2 ^ Y * (1.0 + Z / 4.0)
                // Y in bits [0:5) and Z in bits [5:7)
                float_y = subfield & 0x1F;
                float_z = subfield >> 5;
                result = (1ULL << float_y) * (1.0 + float_z / 4.0);
                break;
            case IMSR::M_FUNCTION_OVERFLOW:
                if (subfield_last > subfield) {
                    ++num_overflow;
                }
                result = subfield + ((m_subfield_max + 1.0) * num_overflow);
                break;
            case IMSR::M_FUNCTION_SCALE:
                result = subfield;
                break;
            default:
                break;
        }
        result *= m_scalar;
        last_field = field;
        return result;
    }

    uint64_t MSREncode::encode(double value)
    {
        uint64_t result = 0;
        double value_inferred = 0.0;
        uint64_t float_y, float_z;
        switch (m_function) {
            case IMSR::M_FUNCTION_SCALE:
                result = (uint64_t)(m_inverse * value);
                break;
            case IMSR::M_FUNCTION_LOG_HALF:
                // F = S * 2.0 ^ -X =>
                // X = log2(S / F)
                result = (uint64_t)std::log2(m_scalar / value);
                break;
            case IMSR::M_FUNCTION_7_BIT_FLOAT:
                // F = S * 2 ^ Y * (1.0 + Z / 4.0)
                // Y in bits [0:5) and Z in bits [5:7)
                if (value > 0) {
                    value *= m_inverse;
                    float_y = (uint64_t)std::log2(value);
                    float_z = (uint64_t)(4.0 * (value / (1 << float_y) - 1.0));
                    if ((float_y && (float_y >> 5) != 0) ||
                        (float_z && (float_z >> 2) != 0)) {
                        throw Exception("MSR::encode(): integer overflow in M_FUNCTION_7_BIT_FLOAT datatype encoding",
                                        EOVERFLOW, __FILE__, __LINE__);
                    }
                    value_inferred = (1 << float_y) * (1.0 + (float_z / 4.0));
                    if ((value - value_inferred) > (value  * 0.25)) {
                        throw Exception("MSR::encode(): inferred value from encoded value is inaccurate",
                                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                    }
                    result = float_y | (float_z << 5);
                }
                else {
                    throw Exception("MSR::encode(): input value <= 0 for M_FUNCTION_7_BIT_FLOAT",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                break;
            case IMSR::M_FUNCTION_OVERFLOW:
                result = (uint64_t)value;
                break;
            default:
                throw Exception("MSR::encode(): unimplemented scale function: " + std::to_string(m_function),
                                GEOPM_ERROR_NOT_IMPLEMENTED,  __FILE__, __LINE__);
                break;
        }
        result = (result << m_shift) & m_mask;
        return result;
    }

    uint64_t MSREncode::mask(void)
    {
        return m_mask;
    }

    int MSREncode::decode_function(void)
    {
        return m_function;
    }

    MSR::MSR(const std::string &msr_name,
             uint64_t offset,
             const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
             const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control)
        : m_name(msr_name)
        , m_offset(offset)
        , m_signal_encode(signal.size(), NULL)
        , m_control_encode(control.size())
        , m_domain_type(IPlatformTopo::M_DOMAIN_INVALID)
        , m_prog_msr(0)
        , m_prog_field_name(0)
        , m_prog_value(0)

    {
        init(signal, control);
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
            m_signal_map.insert(std::pair<std::string, int>(it->first, idx));
            m_signal_encode[idx] = new MSREncode(it->second);
        }
        idx = 0;
        for (auto it = control.begin(); it != control.end(); ++it, ++idx) {
            m_control_map.insert(std::pair<std::string, int>(it->first, idx));
            m_control_encode[idx] = new MSREncode(it->second);
        }
        if (signal.size() != 0) {
            m_domain_type = signal[0].second.domain;
        }
        else if (control.size() != 0) {
            m_domain_type = control[0].second.domain;
        }
        else {
            throw Exception("MSR::init(): both signal and control vectors are empty",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    std::string MSR::name(void) const
    {
        return m_name;
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

    std::string MSR::signal_name(int signal_idx) const
    {
        if (signal_idx < 0 || signal_idx >= num_signal()) {
            throw Exception("MSR::signal_name(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "";
        for (auto it = m_signal_map.begin(); it != m_signal_map.end(); ++it) {
            if (it->second == signal_idx) {
                result = it->first;
                break;
            }
        }
        return result;
    }

    std::string MSR::control_name(int control_idx) const
    {
        if (control_idx < 0 || control_idx >= num_control()) {
            throw Exception("MSR::control_name(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string result = "";
        for (auto it = m_control_map.begin(); it != m_control_map.end(); ++it) {
            if (it->second == control_idx) {
                result = it->first;
                break;
            }
        }
        return result;
    }

    int MSR::signal_index(const std::string &name) const
    {
        int result = -1;
        auto it = m_signal_map.find(name);
        if (it != m_signal_map.end()) {
            result = it->second;
        }
        return result;
    }

    int MSR::control_index(const std::string &name) const
    {
        int result = -1;
        auto it = m_control_map.find(name);
        if (it != m_control_map.end()) {
            result = it->second;
        }
        return result;
    }

    double MSR::signal(int signal_idx,
                       uint64_t field,
                       uint64_t &last_field,
                       uint64_t &num_overflow) const
    {
        if (signal_idx < 0 || signal_idx >= num_signal()) {
            throw Exception("MSR::signal(): signal_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_encode[signal_idx]->decode(field, last_field, num_overflow);
    }

    void MSR::control(int control_idx,
                      double value,
                      uint64_t &field,
                      uint64_t &mask) const
    {
        if (control_idx < 0 || control_idx >= num_control()) {
            throw Exception("MSR::control(): control_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        field = m_control_encode[control_idx]->encode(value);
        mask = m_control_encode[control_idx]->mask();
    }

    uint64_t MSR::mask(int control_idx) const
    {
        return m_control_encode[control_idx]->mask();
    }

    int MSR::domain_type(void) const
    {
        return m_domain_type;
    }

    int MSR::decode_function(int signal_idx) const
    {
        return m_signal_encode[signal_idx]->decode_function();
    }

    MSRSignal::MSRSignal(const IMSR &msr_obj,
                         int domain_type,
                         int cpu_idx,
                         int signal_idx)
        : m_name(msr_obj.name() + ":" + msr_obj.signal_name(signal_idx))
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_signal_idx(signal_idx)
        , m_field_ptr(nullptr)
        , m_field_last(0)
        , m_num_overflow(0)
        , m_is_field_mapped(false)
        , m_is_raw(false)
    {

    }

    MSRSignal::MSRSignal(const IMSR &msr_obj,
                         int domain_type,
                         int cpu_idx)
        : m_name(msr_obj.name() + "#")
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_signal_idx(0)
        , m_field_ptr(nullptr)
        , m_field_last(0)
        , m_num_overflow(0)
        , m_is_field_mapped(false)
        , m_is_raw(true)
    {

    }

    MSRSignal::MSRSignal(const MSRSignal &other)
        : m_name(other.m_name)
        , m_msr_obj(other.m_msr_obj)
        , m_domain_type(other.m_domain_type)
        , m_cpu_idx(other.m_cpu_idx)
        , m_signal_idx(other.m_signal_idx)
        , m_field_ptr(nullptr)
        , m_field_last(other.m_field_last)
        , m_num_overflow(other.m_num_overflow)
        , m_is_field_mapped(false)
        , m_is_raw(other.m_is_raw)
    {

    }

    std::string MSRSignal::name(void) const
    {
        return m_name;
    }

    int MSRSignal::domain_type(void) const
    {
        return m_domain_type;
    }

    int MSRSignal::cpu_idx(void) const
    {
        return m_cpu_idx;
    }

    double MSRSignal::sample(void)
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRSignal::sample(): must call map() method before sample() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        double result = NAN;
        if (!m_is_raw) {
            result = m_msr_obj.signal(m_signal_idx, *m_field_ptr, m_field_last, m_num_overflow);
        }
        else {
            result = geopm_field_to_signal(*m_field_ptr);
        }
        return result;
    }

    uint64_t MSRSignal::offset(void) const
    {
        return m_msr_obj.offset();
    }

    void MSRSignal::map_field(const uint64_t *field)
    {
        m_field_ptr = field;
        m_is_field_mapped = true;
    }

    MSRControl::MSRControl(const IMSR &msr_obj,
                           int domain_type,
                           int cpu_idx,
                           int control_idx)
        : m_name(msr_obj.name() + ":" + msr_obj.control_name(control_idx))
        , m_msr_obj(msr_obj)
        , m_domain_type(domain_type)
        , m_cpu_idx(cpu_idx)
        , m_control_idx(control_idx)
        , m_field_ptr(nullptr)
        , m_mask_ptr(nullptr)
        , m_is_field_mapped(false)
    {

    }

    MSRControl::~MSRControl()
    {

    }

    std::string MSRControl::name() const
    {
        return m_name;
    }

    int MSRControl::domain_type(void) const
    {
        return m_domain_type;
    }

    int MSRControl::cpu_idx(void) const
    {
        return m_cpu_idx;
    }

    void MSRControl::adjust(double setting)
    {
        if (!m_is_field_mapped) {
            throw Exception("MSRControl::adjust(): must call map() method before adjust() can be called",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_msr_obj.control(m_control_idx, setting, *m_field_ptr, *m_mask_ptr);
    }

    uint64_t MSRControl::offset(void) const
    {
        return m_msr_obj.offset();
    }

    uint64_t MSRControl::mask(void) const
    {
        return m_msr_obj.mask(m_control_idx);
    }

    void MSRControl::map_field(uint64_t *field, uint64_t *mask)
    {
        m_field_ptr = field;
        m_mask_ptr = mask;
        m_is_field_mapped = true;
    }
}
