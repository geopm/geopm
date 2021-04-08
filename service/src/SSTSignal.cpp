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
#include "SSTSignal.hpp"

#include "geopm_hash.h"
#include "SSTIO.hpp"

namespace geopm
{

    SSTSignal::SSTSignal(std::shared_ptr<geopm::SSTIO> sstio, SignalType signal_type,
                         int cpu_idx, uint16_t command, uint16_t subcommand,
                         uint32_t subcommand_arg,      // write_value
                         uint32_t interface_parameter) // mbox_interface_param
        : m_sstio(sstio)
        , m_signal_type(signal_type)
        , m_cpu_idx(cpu_idx)
        , m_command(command)
        , m_subcommand(subcommand)
        , m_subcommand_arg(subcommand_arg)
        , m_batch_idx(-1)
    {
    }

    void SSTSignal::setup_batch(void)
    {
        if (m_batch_idx == -1) {
            if (m_signal_type == M_MMIO) {
                m_batch_idx = m_sstio->add_mmio_read(m_cpu_idx, m_subcommand_arg);
            }
            else {
                m_batch_idx = m_sstio->add_mbox_read(
                    m_cpu_idx, m_command, m_subcommand, m_subcommand_arg);
            }
        }
    }

    double SSTSignal::sample(void)
    {
        return geopm_field_to_signal(m_sstio->sample(m_batch_idx));
    }

    double SSTSignal::read(void) const
    {
        uint32_t ret;
        if (m_signal_type == M_MMIO) {
            ret = m_sstio->read_mmio_once(m_cpu_idx, m_subcommand_arg);
        }
        else {
            ret = m_sstio->read_mbox_once(m_cpu_idx, m_command, m_subcommand,
                                          m_subcommand_arg);
        }
        return geopm_field_to_signal(ret);
    }
}
