/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef FFNETAGENT_HPP_INCLUDE
#define FFNETAGENT_HPP_INCLUDE

#include "TensorOneD.hpp"
#include "LocalNeuralNet.hpp"
#include <vector>

#include "geopm/Agent.hpp"
#include "geopm_time.h"
#include "geopm/json11.hpp"

namespace geopm
{
    class PlatformTopo;
    class PlatformIO;
}

/// @brief Agent
class FFNetAgent : public geopm::Agent
{
    public:
        FFNetAgent();
        FFNetAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo);
        virtual ~FFNetAgent() = default;
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
        std::vector<std::function<std::string(double)> > trace_formats(void) const override;
        void trace_values(std::vector<double> &values) override;
        void enforce_policy(const std::vector<double> &policy) const override;

        static std::string plugin_name(void);
        static std::unique_ptr<geopm::Agent> make_plugin(void);
        static std::vector<std::string> policy_names(void);
        static std::vector<std::string> sample_names(void);
    private:
        geopm::PlatformIO &m_platform_io;
        const geopm::PlatformTopo &m_platform_topo;
        geopm_time_s m_last_wait;
        geopm_time_s m_current_time;
        double m_time_delta;
        const double M_WAIT_SEC;
        const int M_NUM_PACKAGE;
        bool m_do_write_batch;

        struct signal
        {
            int batch_idx;
            double signal;
        };

        struct delta_signal
        {
            int batch_idx_num;
            int batch_idx_den;
            double signal_num;
            double signal_den;
            double signal_num_last;
            double signal_den_last;
        };

        struct trace_output
        {
            std::string trace_name;
            double value;
        };

        std::map<std::string, double> m_policy_available;

        std::string m_package_nn_path;
        LocalNeuralNet m_package_neural_net;
        TensorOneD m_last_output;

        std::vector<signal> m_signal_inputs;
        std::vector<delta_signal> m_delta_inputs;

        std::vector<int> m_control_outputs;
        std::vector<std::string> m_trace_outputs;

        int m_sample;

        void init_platform_io(json11::Json nnet_json);
};
#endif
