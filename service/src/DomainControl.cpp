/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "DomainControl.hpp"
#include "geopm/Exception.hpp"
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
