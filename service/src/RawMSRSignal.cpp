/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
        : m_msrio(msrio)
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
