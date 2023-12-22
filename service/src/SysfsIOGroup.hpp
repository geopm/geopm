/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSFSIOGROUP_HPP_INCLUDE
#define SYSFSIOGROUP_HPP_INCLUDE

#include "SysfsDriver.hpp"
#include "UniqueFd.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

#include <functional>

namespace geopm
{

class SaveControl;
class IOUring;

// Arbitrary buffer size. We're generally looking at integer values much shorter
// than 100 digits in length. The IOGroup performs string truncation checks in
// case that ever changes.

class SysfsIOGroup : public IOGroup
{
    public:
        SysfsIOGroup() = delete;
        SysfsIOGroup(std::shared_ptr<SysfsDriver> driver);
        SysfsIOGroup(std::shared_ptr<SysfsDriver> driver,
                     const PlatformTopo &topo,
                     std::shared_ptr<SaveControl> control_saver,
                     std::shared_ptr<IOUring> batch_reader,
                     std::shared_ptr<IOUring> batch_writer);
        virtual ~SysfsIOGroup();
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
        void save_control(const std::string &save_path) override;
        void restore_control(void) override;
        void restore_control(const std::string &save_path) override;
        std::function<double(const std::vector<double> &)> agg_function(const std::string &signal_name) const override;
        std::function<std::string(double)> format_function(const std::string &signal_name) const override;
        std::string signal_description(const std::string &signal_name) const override;
        std::string control_description(const std::string &control_name) const override;
        int signal_behavior(const std::string &signal_name) const override;
        std::string name(void) const override;
    private:
        std::string canonical_name(const std::string &name) const;
        std::string check_request(const std::string &method_name,
                                  const std::string &signal_name,
                                  const std::string &control_name,
                                  int domain_type,
                                  int domain_idx) const;
        std::shared_ptr<SysfsDriver> m_driver;
        const geopm::PlatformTopo &m_platform_topo;
        /// Whether any signal has been pushed
        bool m_do_batch_read;
        /// Whether read_batch() has been called at least once
        bool m_is_batch_read;
        bool m_is_batch_write;
        std::vector<double> m_control_value;
        const std::map<std::string, SysfsDriver::properties_s> m_properties;
        std::map<std::string, std::reference_wrapper<const SysfsDriver::properties_s> > m_signals;
        std::map<std::string, std::reference_wrapper<const SysfsDriver::properties_s> > m_controls;

        // Information about a single pushed signal or control
        struct m_pushed_info_s {
            UniqueFd fd;
            std::string name;
            int domain_type;
            int domain_idx;
            double value;
            bool do_write;
            std::shared_ptr<int> last_io_return;
            std::array<char, SysfsDriver::M_IO_BUFFER_SIZE> buf;
            std::function<double(const std::string&)> parse;
            std::function<std::string(double)> gen;
        };

        // Pushed signals
        std::vector<m_pushed_info_s> m_pushed_info_signal;
        std::vector<m_pushed_info_s> m_pushed_info_control;
        std::shared_ptr<SaveControl> m_control_saver;
        std::shared_ptr<IOUring> m_batch_reader;
        std::shared_ptr<IOUring> m_batch_writer;
};

}

#endif
