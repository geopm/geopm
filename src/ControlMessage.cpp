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

#include "geopm_signal_handler.h"
#include "ControlMessage.hpp"
#include "string.h"

namespace geopm
{

    ControlMessage::ControlMessage(struct geopm_ctl_message_s *ctl_msg, bool is_ctl, bool is_writer)
        : m_ctl_msg(ctl_msg)
        , m_is_ctl(is_ctl)
        , m_is_writer(is_writer)
        , m_last_status(M_STATUS_UNDEFINED)
    {
        memset(m_ctl_msg, 0, sizeof(geopm_ctl_message_s));
    }

    ControlMessage::~ControlMessage()
    {

    }

    void ControlMessage::step(void)
    {
        if (m_is_ctl && m_ctl_msg->ctl_status != M_STATUS_SHUTDOWN) {
            m_ctl_msg->ctl_status++;
        }
        else if (m_is_writer && m_ctl_msg->app_status != M_STATUS_SHUTDOWN) {
            m_ctl_msg->app_status++;
        }
    }

    void ControlMessage::wait(void)
    {
        if (m_last_status != M_STATUS_SHUTDOWN) {
            ++m_last_status;
        }
        while (this_status() != m_last_status) {
            geopm_signal_handler_check();
            if (this_status() == M_STATUS_ABORT) {
                throw Exception("ControlMessage::wait(): Abort sent through control message",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void ControlMessage::abort(void)
    {
        if (m_is_ctl) {
            m_ctl_msg->ctl_status = M_STATUS_ABORT;
        }
        else {
            m_ctl_msg->app_status = M_STATUS_ABORT;
        }
    }

    void ControlMessage::cpu_rank(int cpu_idx, int rank)
    {
        m_ctl_msg->cpu_rank[cpu_idx] = rank;
    }

    int ControlMessage::cpu_rank(int cpu_idx)
    {
        return m_ctl_msg->cpu_rank[cpu_idx];
    }

    bool ControlMessage::is_sample_begin(void)
    {
        return (m_ctl_msg->app_status == M_STATUS_SAMPLE_BEGIN);
    }

    bool ControlMessage::is_sample_end(void)
    {
        return (m_ctl_msg->app_status == M_STATUS_SAMPLE_END);
    }

    bool ControlMessage::is_name_begin(void)
    {
        return (m_ctl_msg->app_status == M_STATUS_NAME_BEGIN);
    }

    bool ControlMessage::is_shutdown(void)
    {
        return (m_ctl_msg->app_status == M_STATUS_SHUTDOWN);
    }

    int ControlMessage::this_status()
    {
        return (m_is_ctl ? m_ctl_msg->app_status : m_ctl_msg->ctl_status);
    }

    void ControlMessage::loop_begin()
    {
        if (m_is_ctl) {
            while (m_ctl_msg->app_status != M_STATUS_NAME_LOOP_BEGIN) {
                geopm_signal_handler_check();
            }
            m_ctl_msg->ctl_status = M_STATUS_NAME_LOOP_BEGIN;
        }
        else {
            m_ctl_msg->app_status = M_STATUS_NAME_LOOP_BEGIN;
            while (m_ctl_msg->ctl_status != M_STATUS_NAME_LOOP_BEGIN) {
                geopm_signal_handler_check();
            }
        }
        m_last_status = M_STATUS_NAME_LOOP_BEGIN;
    }

}
