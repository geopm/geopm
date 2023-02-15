/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "MSRFieldSignal.hpp"

#include "geopm_field.h"
#include "geopm/Exception.hpp"
#include "geopm_debug.hpp"
#include "geopm/Helper.hpp"
#include "MSR.hpp"  // for enums

namespace geopm
{
    MSRFieldSignal::MSRFieldSignal(std::shared_ptr<Signal> raw_msr,
                                   int begin_bit,
                                   int end_bit,
                                   int function,
                                   double scalar)
        : m_raw_msr(raw_msr)
        , m_shift(begin_bit)
        , m_num_bit(end_bit - begin_bit + 1)
        , m_mask(((1ULL << m_num_bit) - 1) << begin_bit)
        , m_subfield_max((1ULL << m_num_bit) - 1)
        , m_function(function)
        , m_scalar(scalar)
        , m_last_field(0)
        , m_num_overflow(0)
        , m_is_batch_ready(false)
    {
        /// @todo: some of these are not logic errors if MSR data
        /// comes from user input files or if this interface is
        /// public. Alternatively, checks for these at the json
        /// parsing step would make these correctly logic errors.
        GEOPM_DEBUG_ASSERT(raw_msr != nullptr,
                           "Signal pointer for raw_msr cannot be null");
        GEOPM_DEBUG_ASSERT(m_num_bit < 64, "64-bit fields are not supported");
        GEOPM_DEBUG_ASSERT(begin_bit <= end_bit,
                           "begin bit must be <= end bit");
        GEOPM_DEBUG_ASSERT(m_function >= 0 && 
                           m_function < MSR::M_NUM_FUNCTION,
                           "invalid encoding function");
    }

    void MSRFieldSignal::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            m_raw_msr->setup_batch();
            m_is_batch_ready = true;
        }
    }

    double MSRFieldSignal::convert_raw_value(double val,
                                     uint64_t &last_field,
                                     int &num_overflow) const
    {
        uint64_t field = geopm_signal_to_field(val);
        uint64_t subfield = (field & m_mask) >> m_shift;
        uint64_t subfield_last = (last_field & m_mask) >> m_shift;
        double result = NAN;

        uint64_t float_y, float_z;
        switch (m_function) {
            case MSR::M_FUNCTION_LOG_HALF:
                // F = S * 2.0 ^ -X
                result = 1.0 / (1ULL << subfield);
                break;
            case MSR::M_FUNCTION_7_BIT_FLOAT:
                // F = S * 2 ^ Y * (1.0 + Z / 4.0)
                // Y in bits [0:5) and Z in bits [5:7)
                float_y = subfield & 0x1F;
                float_z = subfield >> 5;
                result = (1ULL << float_y) * (1.0 + float_z / 4.0);
                break;
            case MSR::M_FUNCTION_OVERFLOW:
                if (subfield_last > subfield) {
                    ++num_overflow;
                }
                result = subfield + ((m_subfield_max + 1.0) * num_overflow);
                break;
            case MSR::M_FUNCTION_SCALE:
                result = subfield;
                break;
            case MSR::M_FUNCTION_LOGIC:
                result = subfield;
                break;
            default:
                GEOPM_DEBUG_ASSERT(false, "invalid function type for MSRFieldSignal");
                break;
        }
        result *= m_scalar;
        last_field = field;
        return result;
    }

    double MSRFieldSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return convert_raw_value(m_raw_msr->sample(), m_last_field, m_num_overflow);
    }

    double MSRFieldSignal::read(void) const
    {
        uint64_t last_field = 0;
        int num_overflow = 0;
        return convert_raw_value(m_raw_msr->read(), last_field, num_overflow);
    }
}
