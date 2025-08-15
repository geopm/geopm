/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "MSRFieldSignal.hpp"

#include <cmath>

#include "geopm_field.h"
#include "geopm/Exception.hpp"
#include "geopm_debug.hpp"
#include "geopm/Helper.hpp"
#include "MSR.hpp"  // for enums
#include "RolloverGenerator.hpp"

namespace geopm
{
    MSRFieldSignal::MSRFieldSignal(std::shared_ptr<Signal> raw_msr,
                                   int begin_bit,
                                   int end_bit,
                                   int function,
                                   double scalar)
        : m_raw_msr(std::move(raw_msr))
        , m_shift(begin_bit)
        , m_num_bit(end_bit - begin_bit + 1)
        , m_mask(((1ULL << m_num_bit) - 1) << begin_bit)
        , m_function(function)
        , m_scalar(scalar)
        , m_is_batch_ready(false)
        , m_rollover_gen(std::make_shared<RolloverGenerator>())
    {
        if (m_num_bit < 64) {
            m_rollover_gen->set_factor(static_cast<double>(1ULL << m_num_bit));
        }
        else {
            m_rollover_gen->set_factor(pow(2, m_num_bit));
        }
        /// @todo: some of these are not logic errors if MSR data
        /// comes from user input files or if this interface is
        /// public. Alternatively, checks for these at the json
        /// parsing step would make these correctly logic errors.
        GEOPM_DEBUG_ASSERT(m_raw_msr != nullptr,
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

    double MSRFieldSignal::convert_raw_value(double val) const
    {
        uint64_t field = geopm_signal_to_field(val);
        uint64_t subfield = (field & m_mask) >> m_shift;
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
                result = m_rollover_gen->update(subfield);
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
        return result;
    }

    double MSRFieldSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return convert_raw_value(m_raw_msr->sample());
    }

    double MSRFieldSignal::read(void) const
    {
        return convert_raw_value(m_raw_msr->read());
    }
}
