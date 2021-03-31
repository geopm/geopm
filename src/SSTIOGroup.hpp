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

#ifndef SSTIOGROUP_HPP_INCLUDE
#define SSTIOGROUP_HPP_INCLUDE

#include <set>
#include <functional>

#include "IOGroup.hpp"


namespace geopm
{
    class PlatformTopo;
    class SSTIO;
    class Signal;
    class Control;

    /// @brief IOGroup that provides a signal for the time since GEOPM startup.
    class SSTIOGroup : public IOGroup
    {
        public:
            SSTIOGroup(const PlatformTopo &topo, std::shared_ptr<SSTIO> sstio);
            virtual ~SSTIOGroup() = default;
            std::set<std::string> signal_names(void) const override;
            std::set<std::string> control_names(void) const override;
            bool is_valid_signal(const std::string &signal_name) const override;
            bool is_valid_control(const std::string &control_name) const override;
            int signal_domain_type(const std::string &signal_name) const override;
            int control_domain_type(const std::string &control_name) const override;
            int push_signal(const std::string &signal_name, int domain_type, int domain_idx)  override;
            int push_control(const std::string &control_name, int domain_type, int domain_idx) override;
            void read_batch(void) override;
            void write_batch(void) override;
            double sample(int batch_idx) override;
            void adjust(int batch_idx, double setting) override;
            double read_signal(const std::string &signal_name, int domain_type, int domain_idx) override;
            void write_control(const std::string &control_name, int domain_type, int domain_idx, double setting) override;
            void save_control(void) override;
            void restore_control(void) override;
            std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
            std::function<std::string(double)> format_function(const std::string &signal_name) const override;
            std::string signal_description(const std::string &signal_name) const override;
            std::string control_description(const std::string &control_name) const override;
            int signal_behavior(const std::string &signal_name) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

            enum class SSTMailboxCommand : uint16_t {
                M_TURBO_FREQUENCY = 0x7f,
                M_CORE_PRIORITY = 0xd0,
                M_SUPPORT_CAPABILITIES = 0x94,
            };
        private:
            struct sst_signal_mailbox_field_s {
                //! Fields for an SST mailbox signal command
                //! @param request_data Data to write to the mailbox prior to
                //!        requesting new data. Often used to indicate which data to
                //!        request for a given subcommand.
                //! @param begin_bit LSB position to read from the output value.
                //! @param end_bit MSB position to read from the output value.
                //! @param multiplier Scaling factor to apply to the read value.
                sst_signal_mailbox_field_s(uint32_t request_data, uint32_t begin_bit,
                                           uint32_t end_bit, double multiplier,
                                           int units, const std::string &description,
                                           m_signal_behavior_e behavior)
                    : request_data(request_data)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                    , behavior(behavior)
                {
                }
                uint32_t request_data;
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
                m_signal_behavior_e behavior;
            };
            struct sst_signal_mailbox_raw_s {
                //! @param command Which type of mailbox command
                //! @param subcommand Subtype of the given command
                //! @param fields Subfields of the mailbox
                sst_signal_mailbox_raw_s(SSTIOGroup::SSTMailboxCommand command, uint16_t subcommand,
                                         const std::map<std::string, sst_signal_mailbox_field_s>& fields)
                    : command(command)
                    , subcommand(subcommand)
                    , fields(fields)
                {
                }
                SSTMailboxCommand command;
                uint16_t subcommand;
                std::map<std::string, sst_signal_mailbox_field_s> fields;
            };

            struct sst_control_mailbox_field_s {
                sst_control_mailbox_field_s(uint32_t write_data, uint32_t begin_bit,
                                            uint32_t end_bit, int units,
                                            const std::string &description)
                    : write_data(write_data)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , units(units)
                    , description(description)
                {
                }
                uint32_t write_data;
                uint32_t begin_bit;
                uint32_t end_bit;
                int units;
                std::string description;
            };
            struct sst_control_mailbox_raw_s {
                //! @param command Which type of mailbox command
                //! @param subcommand Subtype of the given command
                //! @param fields Subfields of the mailbox
                sst_control_mailbox_raw_s(
                    SSTMailboxCommand command, uint16_t subcommand, uint32_t write_param,
                    const std::map<std::string, sst_control_mailbox_field_s>& fields,
                    uint16_t read_subcommand, uint32_t read_request_data)
                    : command(command)
                    , subcommand(subcommand)
                    , write_param(write_param)
                    , fields(fields)
                    , read_subcommand(read_subcommand)
                    , read_request_data(read_request_data)
                {
                }
                SSTMailboxCommand command;
                uint16_t subcommand;
                uint32_t write_param;
                std::map<std::string, sst_control_mailbox_field_s> fields;
                uint16_t read_subcommand;
                uint32_t read_request_data;
            };

            struct sst_control_mmio_field_s {
                sst_control_mmio_field_s(uint32_t begin_bit, uint32_t end_bit,
                                         double multiplier, int units,
                                         const std::string &description)
                    : begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                {
                }
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
            };
            struct sst_control_mmio_raw_s {
                //! @param command Which type of mailbox command
                //! @param subcommand Subtype of the given command
                //! @param fields Subfields of the mailbox
                sst_control_mmio_raw_s(int domain_type, uint32_t register_offset,
                                       const std::map<std::string, sst_control_mmio_field_s>& fields)
                    : domain_type(domain_type)
                    , register_offset(register_offset)
                    , fields(fields)
                {
                }
                int domain_type;
                uint32_t register_offset;
                std::map<std::string, sst_control_mmio_field_s> fields;
            };

            struct sst_signal_mmio_field_s {
                sst_signal_mmio_field_s(uint32_t write_value, uint32_t begin_bit,
                                        uint32_t end_bit, double multiplier,
                                        int units, const std::string &description,
                                        m_signal_behavior_e behavior)
                    : write_value(write_value)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                    , behavior(behavior)
                {
                }
                uint32_t write_value;
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
                m_signal_behavior_e behavior;
            };

            void add_mbox_signals(const std::string &raw_name,
                                  SSTMailboxCommand command, uint16_t subcommand,
                                  const std::map<std::string, sst_signal_mailbox_field_s> &fields);
            void add_mbox_controls(
                const std::string &raw_name, SSTMailboxCommand command,
                uint16_t subcommand, uint32_t write_param,
                const std::map<std::string, sst_control_mailbox_field_s> &fields,
                uint16_t read_subcommand, uint32_t read_request_data,
                uint32_t read_mask);

            void add_mmio_signals(const std::string &raw_name, int domain_type,
                                  uint32_t register_offset,
                                  const std::map<std::string, sst_signal_mmio_field_s> &fields);
            void add_mmio_controls(const std::string &raw_name, int domain_type,
                                   uint32_t register_offset,
                                   const std::map<std::string, sst_control_mmio_field_s> &fields,
                                   uint32_t read_mask);

            static const std::map<std::string, sst_signal_mailbox_raw_s> sst_signal_mbox_info;
            static const std::map<std::string, sst_control_mailbox_raw_s> sst_control_mbox_info;
            static const std::map<std::string, sst_control_mmio_raw_s> sst_control_mmio_info;
            const PlatformTopo &m_topo;
            std::shared_ptr<SSTIO> m_sstio;
            bool m_is_read;

            // All available signals: map from name to signal_info.
            // The signals vector is over the indices for the domain.
            // The signals pointers should be copied when signal is
            // pushed and used directly for read_signal.
            struct signal_info
            {
                std::vector<std::shared_ptr<Signal> > signals;
                int domain;
                int units;
                std::function<double(const std::vector<double> &)> agg_function;
                std::string description;
                m_signal_behavior_e behavior;
            };
            std::map<std::string, signal_info> m_signal_available;

            struct control_info
            {
                std::vector<std::shared_ptr<Control> > controls;
                int domain;
                int units;
                std::function<double(const std::vector<double> &)> agg_function;
                std::string description;
            };
            std::map<std::string, control_info> m_control_available;

            // Mapping of signal index to pushed signals.
            std::vector<std::shared_ptr<Signal> > m_signal_pushed;

            // Mapping of control index to pushed controls
            std::vector<std::shared_ptr<Control> > m_control_pushed;
    };
}

#endif
