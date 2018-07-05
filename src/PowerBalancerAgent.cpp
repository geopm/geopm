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

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "PowerGovernor.hpp"
#include "PowerBalancerAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerBalancerAgent::PowerBalancerAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_level(-1)
        , m_is_converged(false)
        , m_is_sample_stable(false)
        , m_updates_per_sample(5)
        , m_min_power_budget(m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_max_power_budget(m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_power_gov(geopm::make_unique<PowerGovernor> (m_platform_io, m_platform_topo))
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_num_children(0)
        , m_is_root(false)
        , m_last_power_budget_in(NAN)
        , m_last_power_budget_out(NAN)
        , m_epoch_runtime_buf(geopm::make_unique<CircularBuffer<double> >(16)) // Magic number...
        , m_epoch_power_buf(geopm::make_unique<CircularBuffer<double> >(16)) // Magic number...
        , m_sample(M_PLAT_NUM_SIGNAL)
        , m_last_energy_status(0.0)
        , m_ascend_count(0)
        , m_ascend_period(10)
        , m_is_updated(false)
        , m_convergence_target(0.01)
        , m_num_out_of_range(0)
        , m_min_num_converged(15)
        , m_num_converged(0)
        , m_last_epoch_count(0)
        , m_adjusted_power(0.0)
    {

    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        m_level = level;
        if (m_level == 0) {
            init_platform_io(); // Only do this at the leaf level.
        }
        m_num_children = fan_in[level];
        m_is_root = is_root;
        m_last_runtime0.resize(m_num_children, NAN);
        m_last_runtime1.resize(m_num_children, NAN);
        m_last_budget0.resize(m_num_children, NAN);
        m_last_budget1.resize(m_num_children, NAN);
    }

    void PowerBalancerAgent::init_platform_io(void)
    {
        m_power_gov->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        //m_pio_idx[M_PLAT_SIGNAL_EPOCH_ENERGY] = m_platform_io.push_signal("EPOCH_ENERGY", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT] = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_PKG_POWER] = m_platform_io.push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_DRAM_POWER] = m_platform_io.push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);

        // Setup sample aggregation for data going up the tree
        m_agg_func.push_back(IPlatformIO::agg_max);     // EPOCH_RUNTIME
        m_agg_func.push_back(IPlatformIO::agg_average); // POWER
        m_agg_func.push_back(IPlatformIO::agg_and);     // IS_CONVERGED
        m_agg_func.push_back(IPlatformIO::agg_average); // POWER_ENFORCED
    }



    bool PowerBalancerAgent::descend_initial_budget(double power_budget_in, std::vector<double> &power_budget_out)
    {
        bool result = false;
        if (!std::isnan(power_budget_in)) {
            // First time down the tree, send the same budget to all children.
            std::fill(power_budget_out.begin(), power_budget_out.end(), power_budget_in);
            m_last_budget0 = power_budget_out;
            m_is_updated = true;
            result = true;
        }
        return result;
    }

    bool PowerBalancerAgent::descend_updated_budget(double power_budget_in, std::vector<double> &power_budget_out)
    {
        double factor = power_budget_in / IPlatformIO::agg_average(m_last_budget0);
        for (auto &it : power_budget_out) {
            it *= factor;
        }
        m_last_budget0 = power_budget_out;
        std::fill(m_last_budget1.begin(), m_last_budget1.end(), NAN);
        m_epoch_runtime_buf->clear();
        m_epoch_power_buf->clear();
        return true;
    }

    bool PowerBalancerAgent::descend_updated_runtimes(double power_budget_in, std::vector<double> &power_budget_out)
    {
        bool result = false;
        if (m_is_sample_stable) {
            // All children have reported convergance in ascend().
            double stddev_child_runtime = IPlatformIO::agg_stddev(m_last_runtime0);
            // We are out of bounds increment out of range counter
            if (m_is_converged && (stddev_child_runtime > m_convergence_target)) {
                ++m_num_out_of_range;
            }
            // We are within bounds.
            else if (!m_is_converged && (stddev_child_runtime < m_convergence_target)) {
                m_num_out_of_range = 0;
                ++m_num_converged;
                if (m_num_converged >= m_min_num_converged) {
                    m_is_converged = true;
                }
            }
            if (m_num_out_of_range >= m_min_num_converged) {
                // All children have reported that they have
                // converged (m_is_sample_stable), but the
                // relative runtimes between the children is not
                // small enough.

                // Reset counter of number of unique samples that
                // have been passed up that reported convergence.
                m_is_converged  = false;
                m_num_converged = 0;
                m_num_out_of_range = 0;
                power_budget_out = split_budget(power_budget_in);
                // Store the budget history
                m_last_budget1 = m_last_budget0;
                m_last_budget0 = power_budget_out;
                // Clear the runtime and power histories since
                // they reflect the previous budget.
                m_epoch_runtime_buf->clear();
                m_epoch_power_buf->clear();
                // The budget is new, so send it down the tree.
                m_is_updated = true;
                result = true;
            }
        }
        m_last_power_budget_in = power_budget_in;
        return result;
    }

    bool PowerBalancerAgent::descend(const std::vector<double> &policy_in, std::vector<std::vector<double> > &policy_out)
    {
        if (policy_in.size() > 1) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): only one power budget was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        bool result = false;
        double power_budget_in = policy_in[M_POLICY_POWER];
        std::vector<double> power_budget_out(m_num_children, NAN);

        if (std::isnan(m_last_power_budget_in)) {
            // Haven't yet recieved a budget split for the first time
            result = descend_initial_budget(power_budget_in, power_budget_out);
        }
        else if (m_last_power_budget_in != power_budget_in) {
            // The incoming power budget has changed, restart the
            // algorithm
            result = descend_updated_budget(power_budget_in, power_budget_out);
        }
        else if (m_ascend_count == 1) {
            // Not the first descent and the runtimes may have been
            // updated.
            result = descend_updated_runtimes(power_budget_in, power_budget_out);
        }
        // Convert power budget vector into a vector of policy vectors
        for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
            policy_out[child_idx][M_POLICY_POWER] = power_budget_out[child_idx];
        }
        return result;
    }


    bool PowerBalancerAgent::ascend(const std::vector<std::vector<double> > &in_sample, std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        m_is_sample_stable = std::all_of(in_sample.begin(), in_sample.end(),
            [](const std::vector<double> &val)
            {
                return val[M_SAMPLE_IS_CONVERGED];
            });

        // If all children report that they are converged for the last
        // ascend period times, then agregate the samples and send
        // them up the tree.
        if (m_is_sample_stable && m_ascend_count == 0) {
            result = true;
            std::vector<double> child_sample(m_num_children);
            for (size_t sig_idx = 0; sig_idx < out_sample.size(); ++sig_idx) {
                for (int child_idx = 0; child_idx < m_num_children; ++child_idx) {
                    child_sample[child_idx] = in_sample[child_idx][sig_idx];
                }
                out_sample[sig_idx] = m_agg_func[sig_idx](child_sample);
            }
        }
        // Increment the ascend counter if the children are stable.
        if (m_is_sample_stable) {
            ++m_ascend_count;
            if (m_ascend_count == m_ascend_period) {
                m_ascend_count = 0;
            }
        }

        // If we see a new runtime reported from all of the children,
        // then update the history.
        bool do_update = true;
        std::vector<double> this_runtime(m_num_children);
        for (int child_idx = 0; child_idx < m_num_children; ++child_idx) {
            this_runtime[child_idx] = in_sample[child_idx][M_SAMPLE_EPOCH_RUNTIME];
            if (std::isnan(this_runtime[child_idx]) ||
                this_runtime[child_idx] == m_last_runtime0[child_idx]) {
                do_update = false;
            }
        }
        if (do_update) {
            m_last_runtime1 = m_last_runtime0;
            m_last_runtime0 = this_runtime;
        }
        return result;
    }

    bool PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (std::isnan(in_policy[M_POLICY_POWER])) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy is NAN.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        bool result = m_power_gov->adjust_platform(in_policy[M_POLICY_POWER], m_adjusted_power);
        if (m_adjusted_power > in_policy[M_POLICY_POWER]) {
            std::cerr << "Warning: <geopm> PowerBalancerAgent node over budget.  Power policy: " << in_policy[M_POLICY_POWER] << "W power enforced: " << m_adjusted_power << "W" << std::endl;
        }
        m_last_power_budget_out = in_policy[M_POLICY_POWER];
        return result;
    }

    bool PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        m_power_gov->sample_platform();
        // Populate sample vector by reading from PlatformIO
        for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SIGNAL; ++sample_idx) {
            m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
        }

        // If all of the ranks have observed a new epoch then update
        // the circular buffers that hold the history.
        if (m_sample[M_PLAT_SIGNAL_EPOCH_COUNT] != m_last_epoch_count) {
            m_epoch_runtime_buf->insert(m_sample[M_PLAT_SIGNAL_EPOCH_RUNTIME]);
            //m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_EPOCH_ENERGY] / m_sample[M_PLAT_SIGNAL_EPOCH_RUNTIME]);

            /// @todo fix me, should be as above, but we need the
            /// EPOCH_ENERGY signal which doens't currently exist
            m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_EPOCH_RUNTIME]);

            // If we have observed more than m_min_num_converged epoch
            // calls then send median filtered time and power values
            // up the tree.
            if (m_epoch_runtime_buf->size() > m_min_num_converged) {
                out_sample[M_SAMPLE_EPOCH_RUNTIME] = IPlatformIO::agg_median(m_epoch_runtime_buf->make_vector());
                out_sample[M_SAMPLE_POWER] = IPlatformIO::agg_median(m_epoch_power_buf->make_vector());
                out_sample[M_SAMPLE_IS_CONVERGED] = true; //(out_sample[M_SAMPLE_POWER] < 1.01 * m_last_power_budget_in);
                out_sample[M_SAMPLE_POWER_ENFORCED] = m_adjusted_power;
                result = true;
            }
            m_last_epoch_count = m_sample[M_PLAT_SIGNAL_EPOCH_COUNT];
        }
        return result;
    }

    void PowerBalancerAgent::wait()
    {
        // Wait for updates to the energy status register
        double curr_energy_status = 0;

        for (int i = 0; i < m_updates_per_sample; ++i) {
            do  {
                curr_energy_status = m_platform_io.read_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
            }
            while (m_last_energy_status == curr_energy_status);
            m_last_energy_status = curr_energy_status;
        }
    }

    std::vector<double> PowerBalancerAgent::split_budget_first(double power_budget_in)
    {
        std::vector<double> budget(m_num_children);
        // We have only one sample, so move our budget a small amount
        // to measure measure slope of runtime vs. power.
        double median_runtime = IPlatformIO::agg_median(m_last_runtime0);
        double total_target = 0.0;
        for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
            if (m_last_runtime0[child_idx] < median_runtime) {
                budget[child_idx] = m_last_budget0[child_idx] - 10;
            }
            else if (m_last_runtime0[child_idx] >= median_runtime) {
                 budget[child_idx] = m_last_budget0[child_idx] + 10;
            }
            total_target += budget[child_idx];
        }
        // In corner cases we may have to modulate the power a bit to
        // stay at the limit.
        if (power_budget_in * m_num_children != total_target) {
            double delta = power_budget_in - total_target / m_num_children;
            for (auto &it : budget) {
                it += delta;
            }
        }
        return budget;
    }

    std::vector<double> PowerBalancerAgent::split_budget_helper(double avg_power_budget,
                                                            double min_power_budget,
                                                            double max_power_budget)
    {
        // Fit a line to runtime as a function of budget for each
        // child.  Find the budget value for each child such that the
        // projected runtime is uniform given the linear model.
        //
        // sum(result) = avg_power_budget * m_num_children
        // time[i] = time[j] for all i and j
        //
        // m[i] = (m_last_runtime1[i] - m_last_runtime0[i]) / (m_last_budget1[i] - m_last_budget0[i])
        // b[i] = m_last_budget0[i] - m_last_runtime0[i] / m[i]
        //
        // time = m[i] * result[i] + b[i]
        // result[i] = (time - b[i]) / m[i]
        // avg_power_budget * m_num_children = sum((time - b[i]) / m[i])
        //                                 = time * sum(1/m[i]) - sum(b[i] / m[i])
        // time = avg_power_budget * m_num_children / (sum(1/m[i]) - sum(b[i] / m[i]))
        // result[i] = ((avg_power_budget * m_num_children / (sum(1/m[i]) - sum(b[i] / m[i]))) - b[i]) / m[i]

        std::vector<double> result(m_num_children);
        std::vector<double> mm(m_num_children);
        std::vector<double> bb(m_num_children);
        double inv_m_sum = 0.0;
        double ratio_sum = 0.0;
        for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
            mm[child_idx] = (m_last_runtime1[child_idx] - m_last_runtime0[child_idx]) /
                            (m_last_budget1[child_idx] - m_last_budget0[child_idx]);
            bb[child_idx] = m_last_budget0[child_idx] - m_last_runtime0[child_idx] / mm[child_idx];
            inv_m_sum += 1.0 / mm[child_idx];
            ratio_sum += bb[child_idx] / mm[child_idx];
        }
        for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
            result[child_idx] = ((avg_power_budget * m_num_children /
                           (inv_m_sum - ratio_sum)) - bb[child_idx]) / mm[child_idx];
            if (result[child_idx] < min_power_budget) {
                result[child_idx] = min_power_budget;
            }
            else if (result[child_idx] > max_power_budget) {
                result[child_idx] = max_power_budget;
            }
            double pool = avg_power_budget * (m_num_children - child_idx) - result[child_idx];
            if (child_idx != m_num_children - 1) {
                avg_power_budget = pool / (m_num_children - child_idx - 1);
            }
        }
        return result;
    }

    std::vector<double> PowerBalancerAgent::split_budget(double avg_power_budget)
    {
        if (avg_power_budget < m_min_power_budget) {
            throw Exception("PowerBalancerAgent::split_budget(): ave_power_budget less than min_power_budget.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<double> result(m_num_children);
        if (std::any_of(m_last_budget1.begin(), m_last_budget1.end(), [](double val) {return std::isnan(val);})) {
            result = split_budget_first(avg_power_budget);
        }
        else if (avg_power_budget == m_min_power_budget) {
            std::fill(result.begin(), result.end(), m_min_power_budget);
        }
        else if (m_epoch_runtime_buf->size() < m_min_num_converged) {
            result = m_last_budget0;
        }
        else {
            std::vector<std::pair<double, int> > indexed_sorted_last_runtime(m_num_children);
            for (int idx = 0; idx != m_num_children; ++idx) {
                indexed_sorted_last_runtime[idx] = std::make_pair(m_last_runtime0[idx], idx);
            }
            std::sort(indexed_sorted_last_runtime.begin(), indexed_sorted_last_runtime.end());
            // note last_runtime[sort_idx] == indexed_sorted_last_runtime[child_idx].first
            std::vector<double> sorted_last_budget0(m_num_children);
            std::vector<double> sorted_last_budget1(m_num_children);
            std::vector<double> sorted_last_runtime0(m_num_children);
            std::vector<double> sorted_last_runtime1(m_num_children);
            for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
                int sort_idx = indexed_sorted_last_runtime[child_idx].second;
                sorted_last_budget0[child_idx] = m_last_budget0[sort_idx];
                sorted_last_budget1[child_idx] = m_last_budget1[sort_idx];
                sorted_last_runtime0[child_idx] = m_last_runtime0[sort_idx];
                sorted_last_runtime1[child_idx] = m_last_runtime1[sort_idx];
            }
            std::vector<double> sorted_result = split_budget_helper(avg_power_budget,
                                                                    m_min_power_budget,
                                                                    m_max_power_budget);
            for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
                int sort_idx = indexed_sorted_last_runtime[child_idx].second;
                result[child_idx] = sorted_result[sort_idx];
            }
        }
        return result;
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_node(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerBalancerAgent::trace_names(void) const
    {
        return {"epoch_runtime",
                "power_package",
                "power_dram",
                "is_converged",
                "power_budget"};
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_sample[M_PLAT_SIGNAL_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_PKG_POWER] = m_sample[M_PLAT_SIGNAL_PKG_POWER];
        values[M_TRACE_SAMPLE_DRAM_POWER] = m_sample[M_PLAT_SIGNAL_DRAM_POWER];
        values[M_TRACE_SAMPLE_IS_CONVERGED] = m_is_converged;
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget_out;
    }

    std::string PowerBalancerAgent::plugin_name(void)
    {
        return "power_balancer";
    }

    std::unique_ptr<Agent> PowerBalancerAgent::make_plugin(void)
    {
        return geopm::make_unique<PowerBalancerAgent>();
    }

    std::vector<std::string> PowerBalancerAgent::policy_names(void)
    {
        return {"POWER"};
    }

    std::vector<std::string> PowerBalancerAgent::sample_names(void)
    {
        return {"EPOCH_RUNTIME", "POWER", "IS_CONVERGED", "POWER_AVERAGE_ENFORCED"};
    }
}
