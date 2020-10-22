/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "DomainControl.hpp"
#include "Exception.hpp"
#include "geopm_debug.hpp"

namespace geopm
{
    DomainControl::DomainControl(const std::vector<std::shared_ptr<Control> > &controls)
        : m_controls(controls)
        , m_is_batch_ready(false)
    {
        for (const auto &ctl : m_controls) {
            if (ctl == nullptr) {
                throw Exception("DomainControl: internal controls cannot be null",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    void DomainControl::setup_batch(void)
    {
        if (!m_is_batch_ready) {
            for (auto &ctl : m_controls) {
                GEOPM_DEBUG_ASSERT(ctl != nullptr, "null Control saved in vector");
                ctl->setup_batch();
            }
            m_is_batch_ready = true;
        }

    }

    void DomainControl::adjust(double value)
    {
        if (!m_is_batch_ready) {
            throw Exception("DomainControl::adjust(): cannot call adjust() before setup_batch()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (auto &ctl : m_controls) {
            GEOPM_DEBUG_ASSERT(ctl != nullptr, "null Control saved in vector");
            ctl->adjust(value);
        }
    }

    void DomainControl::write(double value)
    {
        for (auto &ctl : m_controls) {
            GEOPM_DEBUG_ASSERT(ctl != nullptr, "null Control saved in vector");
            ctl->write(value);
        }
    }

    void DomainControl::save(void)
    {
        for (auto &ctl : m_controls) {
            GEOPM_DEBUG_ASSERT(ctl != nullptr, "null Control saved in vector");
            ctl->save();
        }
    }

    void DomainControl::restore(void)
    {
        for (auto &ctl : m_controls) {
            GEOPM_DEBUG_ASSERT(ctl != nullptr, "null Control saved in vector");
            ctl->restore();
        }
    }
}
