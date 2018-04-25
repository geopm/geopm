/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#ifndef BALANCINGAGENT_HPP_INCLUDE
#define BALANCINGAGENT_HPP_INCLUDE

#include <vector>

#include "Agent.hpp"

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
    class ITreeComm;

    class BalancingAgent : public IAgent
    {
        public:
            enum m_policy_mailbox_idx_e {
                M_POLICY_MAILBOX_POWER,
                M_NUM_POLICY_MAILBOX,
            };
            enum m_sample_mailbox_idx_e {
                M_SAMPLE_MAILBOX_POWER,
                M_SAMPLE_MAILBOX_IS_CONVERGED,
                M_SAMPLE_MAILBOX_EPOCH_RUNTIME,
                M_NUM_SAMPLE_MAILBOX,
            };

            BalancingAgent();
            virtual ~BalancingAgent();
            void init(int level, int num_leaf) override;
            bool descend(const std::vector<double> &in_policy,
                         std::vector<std::vector<double> >&out_policy) override;
            bool ascend(const std::vector<std::vector<double> > &in_signal,
                        std::vector<double> &out_signal) override;
            void adjust_platform(const std::vector<double> &in_policy) override;
            bool sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::string report_header(void) override;
            std::string report_node(void) override;
            std::map<uint64_t, std::string> report_region(void) override;
            std::vector<std::string> trace_names(void) const override;
            void trace_values(std::vector<double> &values) override;
            static std::string plugin_name(void);
            static std::unique_ptr<IAgent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            void split_budget(double total_power_budget,
                              const std::vector<double> &power_used,
                              const std::vector<double> &runtime,
                              std::vector<double> &result);
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            double m_convergence_guard_band;
            int m_level;
            int m_num_children;
            // signals from parent
            int m_power_budget_in_idx;
            // feedback controls to parent
            int m_power_used_out_idx;
            int m_runtime_out_idx;
            int m_is_converged_out_idx;
            // signals from children
            int m_power_used_agg_in_idx;
            int m_runtime_agg_in_idx;
            int m_is_converged_agg_in_idx;
            std::vector<int> m_power_used_in_idx;
            std::vector<int> m_runtime_in_idx;
            // controls to children
            std::vector<int> m_power_budget_out_idx;
            // used to save results of sampling and budget splits
            // between calls to descend() and ascend()
            std::vector<double> m_power_used_in;
            std::vector<double> m_runtime_in;
            std::vector<double> m_is_converged_in;
            std::vector<double> m_power_budget_out;
            bool m_is_converged;
            std::function<double(const std::vector<double>&)> m_agg_fn_is_converged;
            std::function<double(const std::vector<double>&)> m_agg_fn_power;
            std::function<double(const std::vector<double>&)> m_agg_fn_epoch_runtime;
    };
}

#endif
