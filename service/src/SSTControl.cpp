/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include "SSTControl.hpp"

#include "SSTIO.hpp"

namespace geopm
{
    SSTControl::SSTControl(std::shared_ptr<SSTIO> sstio, ControlType control_type,
                           int cpu_idx, uint32_t command, uint32_t subcommand,
                           uint32_t interface_parameter, uint32_t write_value,
                           uint32_t begin_bit, uint32_t end_bit, double scale,
                           uint32_t rmw_subcommand,
                           uint32_t rmw_interface_parameter, uint32_t rmw_read_mask)
        : m_sstio(sstio)
        , m_control_type(control_type)
        , m_cpu_idx(cpu_idx)
        , m_command(command)
        , m_subcommand(subcommand)
        , m_interface_parameter(interface_parameter)
        , m_write_value(write_value)
        , m_adjust_idx(0)
        , m_shift(begin_bit)
        , m_num_bit(end_bit - begin_bit + 1)
        , m_mask(((1ULL << m_num_bit) - 1) << begin_bit)
        , m_rmw_subcommand(rmw_subcommand)
        , m_rmw_interface_parameter(rmw_interface_parameter)
        , m_rmw_read_mask(rmw_read_mask)
        , m_multiplier(scale)
        , m_saved_value(0)
    {

    }

    void SSTControl::setup_batch(void)
    {
        if (m_control_type == M_MMIO) {
            m_adjust_idx = m_sstio->add_mmio_write(
                m_cpu_idx, m_interface_parameter, m_write_value, m_rmw_read_mask);
        }
        else {
            m_adjust_idx = m_sstio->add_mbox_write(
                m_cpu_idx, m_command, m_subcommand, m_interface_parameter,
                m_rmw_subcommand, m_rmw_interface_parameter, m_rmw_read_mask);
        }
    }

    void SSTControl::adjust(double value)
    {
        m_sstio->adjust(m_adjust_idx,
                        static_cast<uint64_t>(value * m_multiplier) << m_shift,
                        m_mask);
    }

    void SSTControl::write(double value)
    {
        if (m_control_type == M_MMIO) {
            m_sstio->write_mmio_once(
                m_cpu_idx, m_interface_parameter, m_write_value, m_rmw_read_mask,
                static_cast<uint64_t>(value * m_multiplier) << m_shift, m_mask);
        }
        else {
            m_sstio->write_mbox_once(
                m_cpu_idx, m_command, m_subcommand, m_interface_parameter,
                m_rmw_subcommand, m_rmw_interface_parameter, m_rmw_read_mask,
                static_cast<uint64_t>(value * m_multiplier) << m_shift, m_mask);
        }
    }

    void SSTControl::save(void)
    {
        if (m_control_type == M_MMIO) {
            m_saved_value = m_sstio->read_mmio_once(m_cpu_idx, m_interface_parameter);
        }
        else {
            m_saved_value = m_sstio->read_mbox_once(
                m_cpu_idx, m_command, m_rmw_subcommand,
                /* Additional arguments for write operations are used as the
                 * interface parameter. But in read operations, it is preloaded
                 * into the data field to specify which data to read from the
                 * mailbox.
                 */
                m_rmw_interface_parameter);
        }
        m_saved_value &= m_mask;
    }

    void SSTControl::restore(void)
    {
        if (m_control_type == M_MMIO) {
            m_sstio->write_mmio_once(
                m_cpu_idx, m_interface_parameter, m_write_value, m_rmw_read_mask,
                m_saved_value, m_mask);
        }
        else {
            m_sstio->write_mbox_once(
                m_cpu_idx, m_command, m_subcommand, m_interface_parameter,
                m_rmw_subcommand, m_rmw_interface_parameter, m_rmw_read_mask,
                m_saved_value, m_mask);
        }
    }
}
