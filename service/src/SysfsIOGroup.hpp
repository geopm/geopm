/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSFSIOGROUP_HPP_INCLUDE
#define SYSFSIOGROUP_HPP_INCLUDE

#inclue "geopm/IOGroup.hpp"

namespace geopm
{

class SysfsDriver;

class SysfsIOGroup : public IOGroup
{
    public:
        SysfsIOGroup() = delete;
        SysfsIOGroup(std::shared_ptr<SysfsDriver> driver);
        SysfsIOGroup(const PlatformTopo &topo,
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
        static std::string plugin_name(void);
        static std::unique_ptr<geopm::IOGroup> make_plugin(void);
    private:
        const geopm::PlatformTopo &m_platform_topo;
        /// Whether any signal has been pushed
        bool m_do_batch_read;
        /// Whether read_batch() has been called at least once
        bool m_is_batch_read;
        bool m_is_batch_write;
        std::vector<double> m_control_value;

        // Information about a type of signal
        // TODO: merge signal and control structures, and add a "writable" field, like in MSRIO
        struct m_signal_type_info_s {
            // Sysfs attribute name
            std::string attribute;
            double scaling_factor;
            std::string description;
            std::function<double(const std::vector<double> &)> aggregation_function;
            std::function<std::string(double)> format_function;
            int behavior;
            m_units_e units;
            bool is_writable;
        };

        static std::vector<SysfsIOGroup::m_signal_type_info_s> parse_json(
                const std::string& json_text);

        const std::vector<struct m_signal_type_info_s> m_signal_type_info;

        // Maps names to indices into m_signal_type_info
        std::map<std::string, unsigned> m_signal_type_by_name;

        // Map of (cpu)->(cpufreq resource)
        std::map<int, std::string> m_cpufreq_resource_by_cpu;

        std::vector<bool> m_do_write;

        // Information about a single pushed signal index
        struct m_signal_info_s {
            int fd;
            unsigned signal_type;
            int cpu;
            double last_value;
            std::shared_ptr<int> last_io_return;
            std::array<char, IO_BUFFER_SIZE> buf;
        };

        // Pushed signals
        std::vector<m_signal_info_s> m_pushed_signal_info;
        std::vector<m_signal_info_s> m_pushed_control_info;
        std::shared_ptr<SaveControl> m_control_saver;
        std::shared_ptr<IOUring> m_batch_reader;
        std::shared_ptr<IOUring> m_batch_writer;
};

}

#endif
