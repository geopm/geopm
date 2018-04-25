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
    template <class type>
    class ICircularBuffer;

    class BalancingAgent : public IAgent
    {
        public:
            /// @todo May be useful for indexing into the vector of policies or samples.
            enum m_policy_e {
                M_POLICY_POWER,
                M_NUM_POLICY,
            };
            enum m_plat_sample_e {
                M_PLAT_SAMPLE_EPOCH_RUNTIME,
                M_PLAT_SAMPLE_PKG_POWER,
                M_PLAT_SAMPLE_DRAM_POWER,
                M_PLAT_NUM_SAMPLE,
            };
            enum m_trace_sample_e {
                M_TRACE_SAMPLE_EPOCH_RUNTIME,
                M_TRACE_SAMPLE_PKG_POWER,
                M_TRACE_SAMPLE_DRAM_POWER,
                M_TRACE_SAMPLE_IS_CONVERGED,
                M_TRACE_SAMPLE_PWR_BUDGET,
                M_TRACE_NUM_SAMPLE,
            };
            enum m_sample_e { // Tree samples
                M_SAMPLE_EPOCH_RUNTIME,
                M_SAMPLE_POWER,
                M_SAMPLE_IS_CONVERGED,
                M_NUM_SAMPLE,
            };

            BalancingAgent();
            virtual ~BalancingAgent();
            void init(int level, int num_leaf) override;
            bool descend(const std::vector<double> &in_policy,
                         std::vector<std::vector<double> >&out_policy) override;
            bool ascend(const std::vector<std::vector<double> > &in_sample,
                        std::vector<double> &out_sample) override;
            void adjust_platform(const std::vector<double> &in_policy) override;
            bool sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::vector<std::pair<std::string, std::string> > report_header(void) override;
            std::vector<std::pair<std::string, std::string> > report_node(void) override;
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) override;
            std::vector<std::string> trace_names(void) const override;
            void trace_values(std::vector<double> &values) override;
            static std::string plugin_name(void);
            static std::unique_ptr<IAgent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);
        private:
            void init_platform_io(void);
            void runtime_ratio_calc(int offset, double mean_child_runtime,
                                    const std::vector<std::pair<double, int> > &child_runtime,
                                    std::vector<double> &epoch_runtime_ratio,
                                    double &ratio_total,
                                    double &median_epoch_runtime);
            void split_budget(double total_power_budget,
                              const std::vector<double> &power_used,
                              const std::vector<double> &runtime,
                              std::vector<double> &result);
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;

            size_t m_num_tree_sample;
            const double m_convergence_guard_band;
            int m_level; // Needed in order to determine convergence
            int m_num_leaf;
            bool m_is_converged;

            int m_updates_per_sample;
            int m_samples_per_control;
            double m_lower_bound;
            double m_upper_bound;

            std::vector<int> m_pio_idx;

            std::vector<int> m_control_idx;

            std::vector<std::function<double(const std::vector<double>&)> > m_agg_func;

            double m_last_power_budget;
            std::vector<std::vector<double> > m_last_sample;
            std::vector<std::vector<double> > m_last_child_policy;
            std::unique_ptr<ICircularBuffer<double> > m_epoch_runtime_buf;
            std::vector<double> m_sample;

            double m_last_energy_status;
            int m_sample_count;

            bool m_is_updated;
            const double m_convergence_target;
            int m_num_out_of_range;
            int m_min_num_converged;
            int m_num_converged;
            const double m_magic;
            const int m_num_sample;
            int m_last_epoch_count;

    };
}

#endif
