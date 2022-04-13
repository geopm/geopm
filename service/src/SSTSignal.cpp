/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "SSTSignal.hpp"

#include "SSTIO.hpp"
#include "geopm_field.h"

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
