/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef CPUTORCHAGENT_HPP_INCLUDE
#define CPUTORCHAGENT_HPP_INCLUDE

#include <torch/script.h>
#include <vector>

#include "geopm/Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
}

/// @brief Agent
class CPUTorchAgent : public geopm::Agent
{
    public:
        CPUTorchAgent();
        CPUTorchAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo);
        virtual ~CPUTorchAgent() = default;
        void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
        void validate_policy(std::vector<double> &in_policy) const override;
        void split_policy(const std::vector<double> &in_policy,
                          std::vector<std::vector<double> > &out_policy) override;
        bool do_send_policy(void) const override;
        void aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                              std::vector<double> &out_sample) override;
        bool do_send_sample(void) const override;
        void adjust_platform(const std::vector<double> &in_policy) override;
        bool do_write_batch(void) const override;
        void sample_platform(std::vector<double> &out_sample) override;
        void wait(void) override;
        std::vector<std::pair<std::string, std::string> > report_header(void) const override;
        std::vector<std::pair<std::string, std::string> > report_host(void) const override;
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
        std::vector<std::string> trace_names(void) const override;
        void trace_values(std::vector<double> &values) override;
        std::vector<std::function<std::string(double)> > trace_formats(void) const override;

        static std::string plugin_name(void);
        static std::unique_ptr<geopm::Agent> make_plugin(void);
        static std::vector<std::string> policy_names(void);
        static std::vector<std::string> sample_names(void);
    private:
        geopm::PlatformIO &m_platform_io;
        const geopm::PlatformTopo &m_platform_topo;
        geopm_time_s m_last_wait;
        const double M_WAIT_SEC;
        const double M_POLICY_PHI_DEFAULT;
        const int M_NUM_PACKAGE;
        bool m_do_write_batch;

        struct signal
        {
            int batch_idx;
            double signal;
            double sample;
        };

        struct control
        {
            int batch_idx;
            double last_setting;
        };

        // Policy indices; must match policy_names()
        enum m_policy_e {
            M_POLICY_CPU_FREQ_MIN,
            M_POLICY_CPU_FREQ_MAX,
            M_POLICY_CPU_PHI,
            M_NUM_POLICY
        };

        // Sample indices; must match sample_names()
        enum m_sample_e {
            M_NUM_SAMPLE
        };

        std::map<std::string, double> m_policy_available;

        double m_package_frequency_requests;
        std::string m_package_nn_path;
        std::vector<torch::jit::script::Module> m_package_neural_net;

        std::vector<control> m_package_freq_control;
        std::vector<signal> m_package_power;
        std::vector<signal> m_package_freq_status;
        std::vector<signal> m_package_temperature;
        std::vector<signal> m_package_uncore_freq_status;
        std::vector<signal> m_package_qm_rate;
        std::vector<signal> m_package_inst_retired;
        std::vector<signal> m_package_cycles_unhalted;
        std::vector<signal> m_package_energy;
        std::vector<signal> m_package_acnt;
        std::vector<signal> m_package_mcnt;
        std::vector<signal> m_package_pcnt;

        void init_platform_io(void);
};
#endif