/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "MSRFieldControl.hpp"
#include "MSRIO.hpp"
#include "MSR.hpp"
#include "geopm/Exception.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    MSRFieldControl::MSRFieldControl(std::shared_ptr<MSRIO> msrio,
                                     int cpu,
                                     uint64_t offset,
                                     int begin_bit,
                                     int end_bit,
                                     int function,
                                     double scalar)
        : m_msrio(msrio)
        , m_cpu(cpu)
        , m_offset(offset)
        , m_shift(begin_bit)
        , m_num_bit(end_bit - begin_bit + 1)
        , m_mask(((1ULL << m_num_bit) - 1) << begin_bit)
        , m_function(function)
        , m_inverse(1.0 / scalar)
        , m_is_batch_ready(false)
        , m_adjust_idx(-1)
        , m_saved_msr_value(0)
    {
        if (m_msrio == nullptr) {
            throw Exception("MSRFieldControl: cannot construct with null MSRIO",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_function < 0 || m_function >= MSR::M_NUM_FUNCTION ||
            m_function == MSR::M_FUNCTION_OVERFLOW) {
            throw Exception("MSRFieldControl: unsupported encode function.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (begin_bit > end_bit) {
            throw Exception("MSRFieldControl: begin bit must be <= end bit",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void MSRFieldControl::setup_batch(void)
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "null MSRIO");
        if (!m_is_batch_ready) {
            m_adjust_idx = m_msrio->add_write(m_cpu, m_offset);
            m_is_batch_ready = true;
        }
    }

    uint64_t MSRFieldControl::encode(double value) const
    {
        uint64_t result = 0;
        double value_inferred = 0.0;
        uint64_t float_y, float_z;
        switch (m_function) {
            case MSR::M_FUNCTION_SCALE:
                result = (uint64_t)(m_inverse * value);
                break;
            case MSR::M_FUNCTION_LOG_HALF:
                // F = S * 2.0 ^ -X =>
                // X = log2(S / F)
                result = (uint64_t)(-1.0 * std::log2(m_inverse * value));
                break;
            case MSR::M_FUNCTION_7_BIT_FLOAT:
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
                    throw Exception("MSRFieldControl::encode(): input value <= 0 for M_FUNCTION_7_BIT_FLOAT",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                break;
            case MSR::M_FUNCTION_LOGIC:
                result = (uint64_t)(value != 0.0);
                break;
            default:
                GEOPM_DEBUG_ASSERT(false, "unsupported encode function");
                break;
        }
        result = (result << m_shift) & m_mask;
        return result;
    }

    void MSRFieldControl::adjust(double value)
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "null MSRIO");
        if (!m_is_batch_ready) {
            throw Exception("MSRFieldControl::adjust(): cannot call adjust() before setup_batch()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_msrio->adjust(m_adjust_idx, encode(value), m_mask);
    }

    void MSRFieldControl::write(double value)
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "null MSRIO");
        m_msrio->write_msr(m_cpu, m_offset, encode(value), m_mask);
    }

    void MSRFieldControl::save(void)
    {
        m_saved_msr_value = m_msrio->read_msr(m_cpu, m_offset);
        m_saved_msr_value &= m_mask;
    }

    void MSRFieldControl::restore(void)
    {
        m_msrio->write_msr(m_cpu, m_offset, m_saved_msr_value, m_mask);
    }
}
