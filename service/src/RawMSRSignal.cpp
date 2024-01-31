/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "RawMSRSignal.hpp"

#include "geopm_field.h"
#include "geopm_debug.hpp"
#include "MSRIO.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    RawMSRSignal::RawMSRSignal(std::shared_ptr<MSRIO> msrio,
                               int cpu,
                               uint64_t offset)
        : m_msrio(std::move(msrio))
        , m_cpu(cpu)
        , m_offset(offset)
        , m_data_idx(-1)
        , m_is_batch_ready(false)
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "no valid MSRIO object.");
    }

    void RawMSRSignal::setup_batch(void)
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "no valid MSRIO object.");

        if (!m_is_batch_ready) {
            m_data_idx = m_msrio->add_read(m_cpu, m_offset);
            m_is_batch_ready = true;
        }

        GEOPM_DEBUG_ASSERT(m_data_idx != -1, "Signal not added to MSRIO");
    }

    double RawMSRSignal::sample(void)
    {
        if (!m_is_batch_ready) {
            throw Exception("setup_batch() must be called before sample().",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        GEOPM_DEBUG_ASSERT(m_data_idx != -1, "Signal not added to MSRIO");

        // convert to double
        return geopm_field_to_signal(m_msrio->sample(m_data_idx));
    }

    double RawMSRSignal::read(void) const
    {
        GEOPM_DEBUG_ASSERT(m_msrio != nullptr, "no valid MSRIO object.");

        // convert to double
        return geopm_field_to_signal(m_msrio->read_msr(m_cpu, m_offset));
    }
}
