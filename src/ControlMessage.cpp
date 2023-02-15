/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ControlMessage.hpp"

#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include "config.h"

namespace geopm
{

    ControlMessageImp::ControlMessageImp(struct geopm_ctl_message_s &ctl_msg, bool is_ctl, bool is_writer, double wait_sec)
        : M_WAIT_SEC(wait_sec)
        , m_ctl_msg(ctl_msg)
        , m_is_ctl(is_ctl)
        , m_is_writer(is_writer)
        , m_last_status(M_STATUS_UNDEFINED)
    {
        if (!is_ctl && is_writer) {
            memset(&m_ctl_msg, 0, sizeof(geopm_ctl_message_s));
        }
        else {
            bool is_init = false;
            geopm_time_s start;
            geopm_time(&start);
            do {
                if (this_status() == M_STATUS_ABORT) {
                    throw Exception("ControlMessageImp::wait(): Abort sent through control message",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                is_init = (m_ctl_msg.app_status == 0 ||
                           m_ctl_msg.app_status == M_STATUS_MAP_BEGIN);
            } while (!is_init && geopm_time_since(&start) < M_WAIT_SEC);
            if (!is_init) {
                throw Exception("ControlMessageImp::wait(): " + hostname() +
                                " : is_ctl=" + std::to_string(m_is_ctl) +
                                " : is_writer=" + std::to_string(m_is_writer) +
                                " : Timed out waiting for startup",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    void ControlMessageImp::step(void)
    {
        if (m_is_ctl && m_ctl_msg.ctl_status != M_STATUS_SHUTDOWN) {
            m_ctl_msg.ctl_status++;
        }
        else if (m_is_writer && m_ctl_msg.app_status != M_STATUS_SHUTDOWN) {
            m_ctl_msg.app_status++;
        }
    }

    void ControlMessageImp::wait(void)
    {
        if (m_last_status != M_STATUS_SHUTDOWN) {
            ++m_last_status;
        }
        geopm_time_s start;
        geopm_time(&start);
        while (this_status() != m_last_status && geopm_time_since(&start) < M_WAIT_SEC) {
            if (this_status() == M_STATUS_ABORT) {
                throw Exception("ControlMessageImp::wait(): Abort sent through control message",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        if (this_status() != m_last_status) {
            throw Exception("ControlMessageImp::wait(): " + hostname() +
                            " : is_ctl=" + std::to_string(m_is_ctl) +
                            " : is_writer=" + std::to_string(m_is_writer) +
                            " : Timed out waiting for status " + std::to_string(m_last_status),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void ControlMessageImp::abort(void)
    {
        if (m_is_ctl) {
            m_ctl_msg.ctl_status = M_STATUS_ABORT;
        }
        else {
            m_ctl_msg.app_status = M_STATUS_ABORT;
        }
    }

    void ControlMessageImp::cpu_rank(int cpu_idx, int rank)
    {
        m_ctl_msg.cpu_rank[cpu_idx] = rank;
    }

    int ControlMessageImp::cpu_rank(int cpu_idx) const
    {
        return m_ctl_msg.cpu_rank[cpu_idx];
    }

    bool ControlMessageImp::is_sample_begin(void) const
    {
        return (m_ctl_msg.app_status == M_STATUS_SAMPLE_BEGIN);
    }

    bool ControlMessageImp::is_sample_end(void) const
    {
        return (m_ctl_msg.app_status == M_STATUS_SAMPLE_END);
    }

    bool ControlMessageImp::is_name_begin(void) const
    {
        return (m_ctl_msg.app_status == M_STATUS_NAME_BEGIN);
    }

    bool ControlMessageImp::is_shutdown(void) const
    {
        return (m_ctl_msg.app_status == M_STATUS_SHUTDOWN);
    }

    int ControlMessageImp::this_status() const
    {
        return (m_is_ctl ? m_ctl_msg.app_status : m_ctl_msg.ctl_status);
    }

    void ControlMessageImp::loop_begin()
    {
        if (m_is_ctl) {
            while (m_ctl_msg.app_status != M_STATUS_NAME_LOOP_BEGIN) {

            }
            m_ctl_msg.ctl_status = M_STATUS_NAME_LOOP_BEGIN;
        }
        else {
            m_ctl_msg.app_status = M_STATUS_NAME_LOOP_BEGIN;
            while (m_ctl_msg.ctl_status != M_STATUS_NAME_LOOP_BEGIN) {

            }
        }
        m_last_status = M_STATUS_NAME_LOOP_BEGIN;
    }

}
