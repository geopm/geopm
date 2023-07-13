/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSTIOGROUP_HPP_INCLUDE
#define SSTIOGROUP_HPP_INCLUDE

#include <set>
#include <functional>

#include "geopm/IOGroup.hpp"
#include "geopm/Agg.hpp"

namespace geopm
{
    class PlatformTopo;
    class SSTIO;
    class Signal;
    class Control;
    class SaveControl;

    /// @brief IOGroup that provides a signal for the time since GEOPM startup.
    class SSTIOGroup : public IOGroup
    {
        public:
            SSTIOGroup();
            SSTIOGroup(const PlatformTopo &topo,
                       std::shared_ptr<SSTIO> sstio,
                       std::shared_ptr<SaveControl> save_control);
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
            void save_control(const std::string &save_path) override;
            void restore_control(const std::string &save_path) override;
            std::string name(void) const override;
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
                    : sst_signal_mailbox_field_s(
                        request_data,
                        begin_bit,
                        end_bit,
                        multiplier,
                        units,
                        description,
                        behavior,
                        Agg::expect_same)
                {
                }

                sst_signal_mailbox_field_s(uint32_t request_data, uint32_t begin_bit,
                                           uint32_t end_bit, double multiplier,
                                           int units, const std::string &description,
                                           m_signal_behavior_e behavior,
                                           std::function<double(const std::vector<double> &)> agg_function)
                    : request_data(request_data)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                    , behavior(behavior)
                    , agg_function(std::move(agg_function))
                {
                }
                uint32_t request_data;
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
                m_signal_behavior_e behavior;
                std::function<double(const std::vector<double> &)> agg_function;
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
                    : sst_control_mailbox_field_s(
                        write_data,
                        begin_bit,
                        end_bit,
                        units,
                        description,
                        Agg::expect_same)
                {
                }

                sst_control_mailbox_field_s(uint32_t write_data, uint32_t begin_bit,
                                            uint32_t end_bit, int units,
                                            const std::string &description,
                                            std::function<double(const std::vector<double> &)> agg_function)
                    : write_data(write_data)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , units(units)
                    , description(description)
                    , agg_function(std::move(agg_function))
                {
                }
                uint32_t write_data;
                uint32_t begin_bit;
                uint32_t end_bit;
                int units;
                std::string description;
                std::function<double(const std::vector<double> &)> agg_function;
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
                    : sst_control_mmio_field_s(
                        begin_bit,
                        end_bit,
                        multiplier,
                        units,
                        description,
                        Agg::expect_same)
                {
                }
                
                sst_control_mmio_field_s(uint32_t begin_bit, uint32_t end_bit,
                                         double multiplier, int units,
                                         const std::string &description,
                                         std::function<double(const std::vector<double> &)> agg_function)
                    : begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                    , agg_function(std::move(agg_function))
                {
                }
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
                std::function<double(const std::vector<double> &)> agg_function;
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
                    : sst_signal_mmio_field_s(
                        write_value,
                        begin_bit,
                        end_bit,
                        multiplier,
                        units,
                        description,
                        behavior,
                        Agg::expect_same)
                {
                }

                sst_signal_mmio_field_s(uint32_t write_value, uint32_t begin_bit,
                                        uint32_t end_bit, double multiplier,
                                        int units, const std::string &description,
                                        m_signal_behavior_e behavior,
                                        std::function<double(const std::vector<double> &)> agg_function)
                    : write_value(write_value)
                    , begin_bit(begin_bit)
                    , end_bit(end_bit)
                    , multiplier(multiplier)
                    , units(units)
                    , description(description)
                    , behavior(behavior)
                    , agg_function(std::move(agg_function))
                {
                }
                uint32_t write_value;
                uint32_t begin_bit;
                uint32_t end_bit;
                double multiplier;
                int units;
                std::string description;
                m_signal_behavior_e behavior;
                std::function<double(const std::vector<double> &)> agg_function;
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

            std::shared_ptr<SaveControl> m_mock_save_ctl;
    };
}

#endif
