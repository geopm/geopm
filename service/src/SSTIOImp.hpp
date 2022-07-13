/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSTIOIMP_HPP_INCLUDE
#define SSTIOIMP_HPP_INCLUDE

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "SSTIO.hpp"

namespace geopm
{
    class SSTIoctl;
    struct sst_mbox_interface_batch_s;
    struct sst_mmio_interface_batch_s;
    class SSTIOImp : public SSTIO
    {
        public:
            /// @brief Main constructor for the SST ioctl interface.
            /// @param [in] max_cpus The number of CPUs to attempt to map
            ///             to punit cores.
            SSTIOImp(uint32_t max_cpus);

            /// @brief This is the same as the main constructor, but it allows
            /// you to override the ioctl interface.
            SSTIOImp(uint32_t max_cpus, std::shared_ptr<SSTIoctl> ioctl_interface);

            virtual ~SSTIOImp() = default;

            /// Interact with the mailbox on commands that are expected to return data
            int add_mbox_read(uint32_t cpu_index, uint16_t command,
                              uint16_t subcommand, uint32_t subcommand_arg) override;
            int add_mbox_write(uint32_t cpu_index, uint16_t command,
                               uint16_t subcommand, uint32_t interface_parameter,
                               uint16_t read_subcommand,
                               uint32_t read_interface_parameter,
                               uint32_t read_mask) override;
            int add_mmio_read(uint32_t cpu_index, uint16_t register_offset) override;
            int add_mmio_write(uint32_t cpu_index, uint16_t register_offset,
                               uint32_t register_value, uint32_t read_mask) override;
            // call ioctl() for both mbox list and mmio list,
            // unless we end up splitting this class
            void read_batch(void) override;

            uint64_t sample(int batch_idx) const override;
            void write_batch(void) override;
            uint32_t read_mbox_once(uint32_t cpu_index, uint16_t command,
                                    uint16_t subcommand, uint32_t subcommand_arg) override;
            void write_mbox_once(uint32_t cpu_index, uint16_t command,
                                 uint16_t subcommand, uint32_t interface_parameter,
                                 uint16_t read_subcommand,
                                 uint32_t read_interface_parameter,
                                 uint32_t read_mask, uint64_t write_value,
                                 uint64_t write_mask) override;
            uint32_t read_mmio_once(uint32_t cpu_index, uint16_t register_offset) override;
            void write_mmio_once(uint32_t cpu_index, uint16_t register_offset,
                                 uint32_t register_value, uint32_t read_mask,
                                 uint64_t write_value, uint64_t write_mask) override;
            void adjust(int batch_idx, uint64_t write_value, uint64_t write_mask) override;
            uint32_t get_punit_from_cpu(uint32_t cpu_index) override;

        private:
            enum message_type_e
            {
                MBOX,
                MMIO
            };

            template<typename OuterStruct>
            using InnerStruct =
                typename std::remove_all_extents<decltype(OuterStruct::interfaces)>::type;

            // Given a single vector of messages to send to an ioctl, split it
            // into multiple structs to send to that ioctl. Each InnerStruct
            // contains a single message. Each OuterStruct contains multiple
            // messages, with size upper-bounded by m_batch_command_limit.
            template<typename OuterStruct>
            std::vector<std::unique_ptr<OuterStruct, void (*)(OuterStruct *)> >
                ioctl_structs_from_vector(const std::vector<InnerStruct<OuterStruct> > &commands)

            {
                std::vector<std::unique_ptr<OuterStruct, void (*)(OuterStruct *)> > outer_structs;

                size_t handled_commands = 0;
                while (handled_commands < commands.size())
                {
                    size_t batch_size = std::min(
                        static_cast<size_t>(m_batch_command_limit),
                        commands.size() - handled_commands);

                    // The inner struct is embedded in the outer struct, and
                    // the inner struct's size depends on how many entries it
                    // can contain. That size is dynamically determined, so we
                    // manually allocate the outer struct here.
                    outer_structs.emplace_back(reinterpret_cast<OuterStruct *>(
                        new char[sizeof(OuterStruct::num_entries) +
                                 sizeof(InnerStruct<OuterStruct>) * commands.size()]),
                        [](OuterStruct *outer_struct) {
                            delete[] reinterpret_cast<char *>(outer_struct);
                        });
                    outer_structs.back()->num_entries = batch_size;
                    std::copy(commands.data() + handled_commands,
                              commands.data() + handled_commands + batch_size,
                              outer_structs.back()->interfaces);

                    handled_commands += batch_size;
                }

                return outer_structs;
            }

            std::shared_ptr<SSTIoctl> m_ioctl;
            int m_batch_command_limit;
            std::vector<struct sst_mbox_interface_s> m_mbox_read_interfaces;
            std::vector<struct sst_mbox_interface_s> m_mbox_write_interfaces;
            std::vector<struct sst_mbox_interface_s> m_mbox_rmw_interfaces;
            std::vector<uint32_t> m_mbox_rmw_read_masks;
            std::vector<uint32_t> m_mbox_rmw_write_masks;
            std::vector<struct sst_mmio_interface_s> m_mmio_read_interfaces;
            std::vector<struct sst_mmio_interface_s> m_mmio_write_interfaces;
            std::vector<struct sst_mmio_interface_s> m_mmio_rmw_interfaces;
            std::vector<uint32_t> m_mmio_rmw_read_masks;
            std::vector<uint32_t> m_mmio_rmw_write_masks;
            std::vector<std::pair<message_type_e, size_t> > m_added_interfaces;
            std::vector<std::unique_ptr<sst_mbox_interface_batch_s, void(*)(sst_mbox_interface_batch_s*)> > m_mbox_read_batch;
            std::vector<std::unique_ptr<sst_mbox_interface_batch_s, void(*)(sst_mbox_interface_batch_s*)> > m_mbox_write_batch;
            std::vector<std::unique_ptr<sst_mmio_interface_batch_s, void(*)(sst_mmio_interface_batch_s*)> > m_mmio_read_batch;
            std::vector<std::unique_ptr<sst_mmio_interface_batch_s, void(*)(sst_mmio_interface_batch_s*)> > m_mmio_write_batch;
            std::map<uint32_t, uint32_t> m_cpu_punit_core_map;
    };
}

#endif
