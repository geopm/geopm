/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SSTIO.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>

#include <algorithm>
#include <utility>

#include "geopm/Exception.hpp"
#include "SSTIOImp.hpp"
#include "SSTIoctl.hpp"

namespace geopm
{
    std::shared_ptr<SSTIO> SSTIO::make_shared(uint32_t max_cpus)
    {
        return std::make_shared<SSTIOImp>(max_cpus);
    }

    SSTIOImp::SSTIOImp(uint32_t max_cpus)
        : SSTIOImp(max_cpus, SSTIoctl::make_shared("/dev/isst_interface"))
    {
    }

    SSTIOImp::SSTIOImp(uint32_t max_cpus, std::shared_ptr<SSTIoctl> ioctl_interface)
        : m_ioctl(std::move(ioctl_interface))
        , m_batch_command_limit(0)
    {
        sst_version_s sst_version;
        int err = m_ioctl->version(&sst_version);
        if (err == -1) {
            throw Exception("SSTIOImp::SSTIOImp() failed to get the SST driver version information",
                            errno, __FILE__, __LINE__);
        }
        if (!sst_version.is_mbox_supported) {
            throw Exception("SSTIOImp::SSTIOImp() SST driver does not support MBOX messages",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (!sst_version.is_mmio_supported) {
            throw Exception("SSTIOImp::SSTIOImp() SST driver does not support MMIO messages",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (sst_version.batch_command_limit == 0) {
            throw Exception("SSTIOImp::SSTIOImp() SST driver reports 0-command batch size limit",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_batch_command_limit = sst_version.batch_command_limit;

        std::vector<sst_cpu_map_interface_s> batch_read_data;
        batch_read_data.reserve(max_cpus);
        for (uint32_t i = 0; i < max_cpus; ++i) {
            batch_read_data.emplace_back(sst_cpu_map_interface_s{ i, 0 });
        }
        auto batch_reads = ioctl_structs_from_vector<sst_cpu_map_interface_batch_s>(
            batch_read_data);

        for (auto &batch_read : batch_reads) {
            err = m_ioctl->get_cpu_id(batch_read.get());
            if (err == -1) {
                throw Exception("SSTIOImp::SSTIOImp() failed to get CPU map",
                                errno, __FILE__, __LINE__);
            }

            for (size_t i = 0; i < batch_read->num_entries; ++i) {
                m_cpu_punit_core_map.emplace(batch_read->interfaces[i].cpu_index,
                                             // LSB indicates which hyperthread
                                             // is mapped. Right-shift once to
                                             // drop the hyperthread bit
                                             batch_read->interfaces[i].punit_cpu >> 1);
            }
        }
    }

    int SSTIOImp::add_mbox_read(uint32_t cpu_index, uint16_t command,
                                uint16_t subcommand, uint32_t subcommand_arg)
    {
        // save the stuff in the list
        struct sst_mbox_interface_s mbox {
            .cpu_index = cpu_index,
            .mbox_interface_param = 0,
            .write_value = subcommand_arg,
            .read_value = 0,
            .command = command,
            .subcommand = subcommand,
            .reserved = 0
        };

        // Stage everything in a vector for now. It will be copied to the ioctl
        // buffer later.
        int mbox_idx = m_mbox_read_interfaces.size();
        auto it = std::find_if(
            m_mbox_read_interfaces.begin(), m_mbox_read_interfaces.end(),
            [&mbox](const sst_mbox_interface_s &existing_mbox) {
                return existing_mbox.cpu_index == mbox.cpu_index &&
                       existing_mbox.mbox_interface_param == mbox.mbox_interface_param &&
                       existing_mbox.command == mbox.command &&
                       existing_mbox.subcommand == mbox.subcommand &&
                       existing_mbox.write_value == mbox.write_value;
            });

        int idx = -1;
        if (it == m_mbox_read_interfaces.end()) {
            m_mbox_read_interfaces.push_back(mbox);

            // Multiple ioctls with different data structures are used here,
            // along with multiple ioctl buffers. This vector indicates how a
            // signal ID maps to a buffer, and to which offset in that buffer.
            idx = m_added_interfaces.size();
            m_added_interfaces.emplace_back(MBOX, mbox_idx);
        }
        else {
            // This reader has been added before. Return the previously-used
            // signal index.
            size_t read_interface_idx = std::distance(
                m_mbox_read_interfaces.begin(), it);
            auto index_it = std::find(m_added_interfaces.begin(),
                                      m_added_interfaces.end(),
                                      std::make_pair(MBOX, read_interface_idx));
            if (index_it == m_added_interfaces.end()) {
                throw Exception(
                    "SSTIOImp::add_mbox_read(): Inserted an existing "
                    "signal, but cannot find its signal index",
                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
            idx = std::distance(m_added_interfaces.begin(), index_it);
        }

        return idx;
    }

    int SSTIOImp::add_mbox_write(uint32_t cpu_index, uint16_t command,
                                 uint16_t subcommand, uint32_t interface_parameter,
                                 uint16_t read_subcommand,
                                 uint32_t read_interface_parameter, uint32_t read_mask)
    {
        struct sst_mbox_interface_s mbox {
            .cpu_index = cpu_index,
            .mbox_interface_param = interface_parameter,
            .write_value = 0,
            .read_value = 0,
            .command = command,
            .subcommand = subcommand,
            .reserved = 0
        };
        int mbox_idx = m_mbox_write_interfaces.size();
        auto it = std::find_if(
            m_mbox_write_interfaces.begin(), m_mbox_write_interfaces.end(),
            [&mbox](const sst_mbox_interface_s &existing_mbox) {
                return existing_mbox.cpu_index == mbox.cpu_index &&
                       existing_mbox.mbox_interface_param == mbox.mbox_interface_param &&
                       existing_mbox.command == mbox.command &&
                       existing_mbox.subcommand == mbox.subcommand;
            });

        int idx = -1;
        if (it == m_mbox_write_interfaces.end()) {
            // First time this write slot is being added. Track both the
            // actual write parameters and the associated read parameters for
            // read-modify-write.
            m_mbox_write_interfaces.push_back(mbox);

            mbox.mbox_interface_param = read_interface_parameter;
            mbox.subcommand = read_subcommand;
            m_mbox_rmw_interfaces.push_back(mbox);
            m_mbox_rmw_read_masks.push_back(read_mask);
            m_mbox_rmw_write_masks.push_back(0);

            idx = m_added_interfaces.size();
            m_added_interfaces.emplace_back(MBOX, mbox_idx);
        }
        else {
            // This writer, or another in the same mailbox slot, has been
            // added before. Return the previously used control index.
            size_t write_interface_idx = std::distance(
                m_mbox_write_interfaces.begin(), it);
            auto index_it = std::find(m_added_interfaces.begin(),
                                      m_added_interfaces.end(),
                                      std::make_pair(MBOX, write_interface_idx));
            if (index_it == m_added_interfaces.end()) {
                throw Exception(
                    "SSTIOImp::add_mbox_write(): Inserted an existing "
                    "control, but cannot find its control index",
                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
            idx = std::distance(m_added_interfaces.begin(), index_it);
        }

        // Report the control ID as a separate index that encodes both ioctl
        // type and the offset within that ioctl's message buffer.
        return idx;
    }

    int SSTIOImp::add_mmio_read(uint32_t cpu_index, uint16_t register_offset)
    {
        struct sst_mmio_interface_s mmio {
            .is_write = 0,
            .cpu_index = cpu_index,
            .register_offset = register_offset,
            .value = 0,
        };
        int mmio_idx = m_mmio_read_interfaces.size();
        m_mmio_read_interfaces.push_back(mmio);

        int idx = m_added_interfaces.size();
        m_added_interfaces.emplace_back(MMIO, mmio_idx);
        return idx;
    }

    int SSTIOImp::add_mmio_write(uint32_t cpu_index, uint16_t register_offset,
                                 uint32_t register_value, uint32_t read_mask)
    {
        struct sst_mmio_interface_s mmio {
            .is_write = 1,
            .cpu_index = cpu_index,
            .register_offset = register_offset,
            .value = register_value,
        };
        int mmio_idx = m_mmio_write_interfaces.size();
        m_mmio_write_interfaces.push_back(mmio);

        mmio.is_write = 0;
        m_mmio_rmw_interfaces.push_back(mmio);
        m_mmio_rmw_read_masks.push_back(read_mask);
        m_mmio_rmw_write_masks.push_back(0);

        int idx = m_added_interfaces.size();
        m_added_interfaces.emplace_back(MMIO, mmio_idx);
        return idx;
    }

    void SSTIOImp::read_batch(void)
    {
        if (!m_mbox_read_interfaces.empty()) {
            m_mbox_read_batch = ioctl_structs_from_vector<sst_mbox_interface_batch_s>(
                m_mbox_read_interfaces);

            for (auto &batch : m_mbox_read_batch) {
                errno = 0;
                int err = m_ioctl->mbox(batch.get());
                if (err == -1 && errno == EBUSY) {
                    errno = 0;
                    err = m_ioctl->mbox(batch.get());
                }
                if (err == -1) {
                    throw Exception("SSTIOImp::read_batch() mbox read failed",
                                    errno, __FILE__, __LINE__);
                }
            }
        }
        if (!m_mmio_read_interfaces.empty()) {
            m_mmio_read_batch = ioctl_structs_from_vector<sst_mmio_interface_batch_s>(
                m_mmio_read_interfaces);

            for (auto &batch : m_mmio_read_batch) {
                int err = m_ioctl->mmio(batch.get());
                if (err == -1) {
                    throw Exception("SSTIOImp::read_batch() mmio read failed",
                                    errno, __FILE__, __LINE__);
                }
            }
        }
    }

    uint64_t SSTIOImp::sample(int batch_idx) const
    {
        const auto &interface = m_added_interfaces[batch_idx];
        uint64_t sample_value = 0;
        // All interfaces in the list are divided into groups limited by a
        // system-defined maximum size per group of commands. We use division
        // to determine which group contains the requested sample. The modulo
        // operation determines which sample within that group is the one we
        // need.
        if (interface.first == MMIO) {
            size_t mmio_batch_idx = interface.second / m_batch_command_limit;
            size_t mmio_interface_idx = interface.second % m_batch_command_limit;
            sample_value = m_mmio_read_batch[mmio_batch_idx]->interfaces[mmio_interface_idx].value;
        }
        else {
            if (interface.first != MBOX) {
                throw Exception("SSTIOImp::sample(): Unexpected interface type",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
            size_t mbox_batch_idx = interface.second / m_batch_command_limit;
            size_t mbox_interface_idx = interface.second % m_batch_command_limit;
            sample_value = m_mbox_read_batch[mbox_batch_idx]->interfaces[mbox_interface_idx].read_value;
        }
        return sample_value;
    }

    void SSTIOImp::write_batch(void)
    {
        if (!m_mbox_write_interfaces.empty()) {
            m_mbox_write_batch = ioctl_structs_from_vector<sst_mbox_interface_batch_s>(
                m_mbox_rmw_interfaces);

            for (auto &batch : m_mbox_write_batch) {
                // Read existing value (TODO: only need if not whole mask write)
                errno = 0;
                int err = m_ioctl->mbox(batch.get());
                if (err == -1 && errno == EBUSY) {
                    errno = 0;
                    err = m_ioctl->mbox(batch.get());
                }
                if (err == -1) {
                    throw Exception("sstioimp::write_batch() pre-write mbox read failed",
                                    errno, __FILE__, __LINE__);
                }

                // Modify the existing values with the adjusted values, using
                // the buffer that contains the mailbox write locations (which
                // may be different from the read locations for some controls)
                for (size_t i = 0; i < batch->num_entries; ++i) {
                    // Mask the read so we only propagate the bits that we are
                    // supposed to read. Mask the write so we only update the
                    // adjusted bits.
                    m_mbox_write_interfaces[i].write_value |=
                        ~m_mbox_rmw_write_masks[i] &
                        (batch->interfaces[i].read_value &
                         m_mbox_rmw_read_masks[i]);
                }
            }

            m_mbox_write_batch = ioctl_structs_from_vector<sst_mbox_interface_batch_s>(
                m_mbox_write_interfaces);

            for (auto &batch : m_mbox_write_batch) {
                // Write the adjusted value
                errno = 0;
                int err = m_ioctl->mbox(batch.get());
                if (err == -1 && errno == EBUSY) {
                    errno = 0;
                    err = m_ioctl->mbox(batch.get());
                }
                if (err == -1) {
                    throw Exception("sstioimp::write_batch() mbox write failed",
                                    errno, __FILE__, __LINE__);
                }
            }
        }

        if (!m_mmio_write_interfaces.empty()) {
            m_mmio_write_batch = ioctl_structs_from_vector<sst_mmio_interface_batch_s>(
                m_mmio_rmw_interfaces);

            for (auto &batch : m_mmio_write_batch) {
                // Read existing value (TODO: only need if not whole mask write)
                int err = m_ioctl->mmio(batch.get());
                if (err == -1) {
                    throw Exception("sstioimp::write_batch() pre-write mmio read failed",
                                    errno, __FILE__, __LINE__);
                }

                // Modify the existing values with the adjusted values, using
                // the buffer that contains the mailbox write locations (which
                // may be different from the read locations for some controls)
                for (size_t i = 0; i < batch->num_entries; ++i) {
                    // Mask the read so we only propagate the bits that we are
                    // supposed to read. Mask the write so we only update the
                    // adjusted bits.
                    m_mmio_write_interfaces[i].value |=
                        ~m_mmio_rmw_write_masks[i] &
                        (batch->interfaces[i].value &
                         m_mmio_rmw_read_masks[i]);
                }
            }

            m_mmio_write_batch = ioctl_structs_from_vector<sst_mmio_interface_batch_s>(
                m_mmio_write_interfaces);

            for (auto &batch : m_mmio_write_batch) {
                // Write the adjusted value
                int err = m_ioctl->mmio(batch.get());
                if (err == -1) {
                    throw Exception("sstioimp::write_batch() mmio write failed",
                                    errno, __FILE__, __LINE__);
                }
            }
        }
    }

    uint32_t SSTIOImp::read_mbox_once(uint32_t cpu_index, uint16_t command,
                                      uint16_t subcommand, uint32_t subcommand_arg)
    {
        sst_mbox_interface_batch_s read_batch{
            .num_entries = 1,
            .interfaces = { { .cpu_index = cpu_index,
                              .mbox_interface_param = 0,
                              .write_value = subcommand_arg,
                              .read_value = 0,
                              .command = command,
                              .subcommand = subcommand,
                              .reserved = 0 } }
        };

        errno = 0;
        int err = m_ioctl->mbox(&read_batch);
        if (err == -1 && errno == EBUSY) {
            err = m_ioctl->mbox(&read_batch);
        }
        if (err == -1) {
            throw Exception("sstioimp::read_mbox_once() mbox read failed",
                            errno, __FILE__, __LINE__);
        }

        return read_batch.interfaces[0].read_value;
    }

    void SSTIOImp::write_mbox_once(uint32_t cpu_index, uint16_t command,
                                   uint16_t subcommand, uint32_t interface_parameter,
                                   uint16_t read_subcommand,
                                   uint32_t read_interface_parameter, uint32_t read_mask,
                                   uint64_t write_value, uint64_t write_mask)
    {
        sst_mbox_interface_batch_s batch{
            .num_entries = 1,
            .interfaces = { { .cpu_index = cpu_index,
                              .mbox_interface_param = read_interface_parameter,
                              .write_value = 0,
                              .read_value = 0,
                              .command = command,
                              .subcommand = read_subcommand,
                              .reserved = 0 } }
        };
        errno = 0;
        int err = m_ioctl->mbox(&batch);
        if (err == -1 && errno == EBUSY) {
            err = m_ioctl->mbox(&batch);
        }
        if (err == -1) {
            throw Exception("sstioimp::write_mbox_once() pre-write mbox read failed",
                            errno, __FILE__, __LINE__);
        }
        batch.interfaces[0].write_value =
            write_value |
            (~write_mask & (batch.interfaces[0].read_value & read_mask));
        batch.interfaces[0].mbox_interface_param = interface_parameter;
        batch.interfaces[0].read_value = 0;
        batch.interfaces[0].subcommand = subcommand;

        errno = 0;
        err = m_ioctl->mbox(&batch);
        if (err == -1 && errno == EBUSY) {
            err = m_ioctl->mbox(&batch);
        }
        if (err == -1) {
            throw Exception("sstioimp::write_mbox_once() mbox write failed",
                            errno, __FILE__, __LINE__);
        }
    }

    uint32_t SSTIOImp::read_mmio_once(uint32_t cpu_index, uint16_t register_offset)
    {
        sst_mmio_interface_batch_s read_batch{
            .num_entries = 1,
            .interfaces = { { .is_write = 0,
                              .cpu_index = cpu_index,
                              .register_offset = register_offset,
                              .value = 0 } }
        };

        int err = m_ioctl->mmio(&read_batch);
        if (err == -1) {
            throw Exception("sstioimp::read_mmio_once() mmio read failed",
                            errno, __FILE__, __LINE__);
        }

        return read_batch.interfaces[0].value;
    }

    void SSTIOImp::write_mmio_once(uint32_t cpu_index, uint16_t register_offset,
                                   uint32_t register_value, uint32_t read_mask,
                                   uint64_t write_value, uint64_t write_mask)
    {
        sst_mmio_interface_batch_s batch{
            .num_entries = 1,
            .interfaces = { { .is_write = 0,
                              .cpu_index = cpu_index,
                              .register_offset = register_offset,
                              .value = 0 } }
        };

        int err = m_ioctl->mmio(&batch);
        if (err == -1) {
            throw Exception("sstioimp::write_mmio_once() pre-write mmio read failed",
                            errno, __FILE__, __LINE__);
        }

        batch.interfaces[0].is_write = 1;
        batch.interfaces[0].value =
            write_value | (~write_mask & (batch.interfaces[0].value & read_mask));

        err = m_ioctl->mmio(&batch);
        if (err == -1) {
            throw Exception("sstioimp::write_mmio_once() mmio write failed",
                            errno, __FILE__, __LINE__);
        }
    }

    void SSTIOImp::adjust(int batch_idx, uint64_t write_value, uint64_t write_mask)
    {
        const auto &interface = m_added_interfaces[batch_idx];
        auto &write_destination =
            interface.first == MMIO
                ? m_mmio_write_interfaces[interface.second].value
                : m_mbox_write_interfaces[interface.second].write_value;
        write_destination &= ~write_mask;
        write_destination |= write_value;

        // Update the write masks so we know which bits to use in the write
        // phase of the ioctl RMW operations.
        if (interface.first == MBOX) {
            m_mbox_rmw_write_masks[interface.second] |= write_mask;
        }
        else {
            m_mmio_rmw_write_masks[interface.second] |= write_mask;
        }
    }

    uint32_t SSTIOImp::get_punit_from_cpu(uint32_t cpu_index)
    {
        return m_cpu_punit_core_map.at(cpu_index);
    }
}
