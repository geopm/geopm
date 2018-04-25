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

#include "BalancingAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    BalancingAgent::BalancingAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_convergence_guard_band(0.5)
        , m_level(-1)
        , m_num_leaf(-1)
        , m_is_converged(false)
        , m_updates_per_sample(5)
        , m_samples_per_control(10)
        , m_lower_bound(m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_upper_bound(m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_pio_idx(M_PLAT_NUM_SAMPLE)
        , m_last_power_budget(-DBL_MAX)
        , m_epoch_runtime_buf(geopm::make_unique<CircularBuffer<double> >(8)) // Magic number...
        , m_sample(M_PLAT_NUM_SAMPLE)
        , m_last_energy_status(0.0)
        , m_update_count(0)
        , m_sample_count(0)
        , m_is_updated(false)
        , m_convergence_target(0.01)
        , m_num_out_of_range(0)
        , m_min_num_converged(7)
        , m_num_converged(0)
        , m_magic(3.0) // m_slope_modifier from BalancingDecider
        , m_num_sample(3) // Number of samples required to be in m_epoch_runtime_buf before balancing begins
        , m_last_epoch_count(0)
    {

    }

    BalancingAgent::~BalancingAgent()
    {

    }

    void BalancingAgent::init(int level, int num_leaf)
    {
        m_level = level;
        m_num_leaf = num_leaf;
        if (level == 0) {
            init_platform_io(); // Only do this at the leaf level.
        }
    }

    void BalancingAgent::init_platform_io(void)
    {
        // Setup signals
        m_pio_idx[M_PLAT_SAMPLE_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SAMPLE_PKG_POWER] = m_platform_io.push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SAMPLE_DRAM_POWER] = m_platform_io.push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);

        // Setup controls
        int pkg_pwr_domain_type = m_platform_io.control_domain_type("POWER_PACKAGE");
        if (pkg_pwr_domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): Platform does not support package power control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }

        int num_pkg_pwr_domains = m_platform_topo.num_domain(pkg_pwr_domain_type);
        for(int i = 0; i < num_pkg_pwr_domains; ++i) {
            int control_idx = m_platform_io.push_control("POWER_PACKAGE", pkg_pwr_domain_type, i);
            if (control_idx < 0) {
                throw Exception("BalancingAgent::" + std::string(__func__) + "(): Failed to enable package power control"
                                " in the platform.",
                                GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
            }
            m_control_idx.push_back(control_idx);
        }

        // Setup sample aggregation for data going up the tree
        m_agg_func.push_back(IPlatformIO::agg_max);     // EPOCH_RUNTIME
        m_agg_func.push_back(IPlatformIO::agg_average); // POWER
        m_agg_func.push_back(IPlatformIO::agg_and);     // IS_CONVERGED
    }

    bool BalancingAgent::descend(const std::vector<double> &in_policy, std::vector<std::vector<double> >&out_policy)
    {
        // @todo This entire function is a candidate for major cleanup/refactor.
        if (in_policy.size() > 1) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): only one power budget was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        int num_children = out_policy.size();
        double avg_per_node_pwr_tgt = in_policy[0];
        m_is_updated = (m_level == 0);

        if (m_last_power_budget != avg_per_node_pwr_tgt) {
            double stddev_child_runtime = 0.0;

            if (m_last_power_budget == -DBL_MAX) {
                // First time down the tree, send the budget to all children.
                std::fill(out_policy.begin(), out_policy.end(), std::vector<double>(1, avg_per_node_pwr_tgt));
                m_is_updated = true;
            }
            else { // Not the first descent
                if (m_epoch_runtime_buf->size() >= m_num_sample) {

                    std::vector<std::pair<double, int> > child_runtime(num_children);
                    double mean_child_runtime = 0.0;
                    double child_runtime_sum = 0.0;
                    for (int child_idx = 0; child_idx < num_children; ++child_idx) {
                        child_runtime[child_idx].first = m_last_sample[child_idx][M_SAMPLE_EPOCH_RUNTIME];
                        child_runtime[child_idx].second = child_idx;
                        child_runtime_sum += child_runtime[child_idx].first;
                        stddev_child_runtime += child_runtime[child_idx].first * child_runtime[child_idx].first;
                    }
                    mean_child_runtime = child_runtime_sum /  num_children;
                    stddev_child_runtime = sqrt(stddev_child_runtime / num_children -
                                                mean_child_runtime * mean_child_runtime) / mean_child_runtime;

                    // If we are not within bounds redistribute power
                    if (!m_is_converged && stddev_child_runtime > m_convergence_target) {
                        m_num_converged = 0;
                        std::sort(child_runtime.begin(), child_runtime.end());

                        std::vector<double> epoch_runtime_ratio(num_children);
                        double median_epoch_runtime = 0.0;
                        double ratio_total = 0.0;
                        runtime_ratio_calc(0, mean_child_runtime, child_runtime, epoch_runtime_ratio,
                                           ratio_total, median_epoch_runtime);

                        double power_total = m_last_power_budget * m_num_leaf;
                        double target_lower_bound = m_lower_bound * m_num_leaf;
                        double power_sum = 0.0;
                        double runtime_sum = 0.0;
                        for (size_t child_runtime_idx = 0;
                             child_runtime_idx != child_runtime.size();
                             ++child_runtime_idx) {
                            int child_idx = child_runtime[child_runtime_idx].second;
                            double target = power_total * epoch_runtime_ratio[child_idx] / ratio_total;
                            if (target < target_lower_bound) {
                                target = m_lower_bound;
                                power_total -= target + power_sum;
                                child_runtime_sum -= median_epoch_runtime + runtime_sum;
                                power_sum = 0.0;
                                runtime_sum = 0.0;
                                ratio_total = 0.0;
                                runtime_ratio_calc(child_runtime_idx + 1, mean_child_runtime, child_runtime,
                                                   epoch_runtime_ratio, ratio_total, median_epoch_runtime);
                            }
                            else {
                                power_total += target;
                                runtime_sum += median_epoch_runtime;
                            }
                            out_policy[child_idx] = {target};
                        }

                        m_epoch_runtime_buf->clear();
                        m_is_updated = true;
                    }
                }

                if (m_is_converged && (stddev_child_runtime > m_convergence_target)) {
                    ++m_num_out_of_range;
                    if (m_num_out_of_range >= m_min_num_converged) {
                        m_is_converged  = false;
                        m_num_converged = 0;
                        m_num_out_of_range = 0;
                    }
                }
                // We are within bounds.
                else if (!m_is_converged && (stddev_child_runtime < m_convergence_target)) {
                    m_num_out_of_range = 0;
                    ++m_num_converged;
                    if (m_num_converged >= m_min_num_converged) {
                        m_is_converged = true;
                    }
                }
            }

            m_last_power_budget = avg_per_node_pwr_tgt;
            m_last_child_policy = out_policy;
        }

        return m_is_updated;
    }

    bool BalancingAgent::ascend(const std::vector<std::vector<double> > &in_sample, std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        /// @todo Move the below to IAgent::ascend()?  Or similar.
        std::vector<double> child_sample(in_sample.size());
        for (size_t sig_idx = 0; sig_idx < out_sample.size(); ++sig_idx) {
            for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
                child_sample[child_idx] = in_sample[child_idx][sig_idx];
            }
            out_sample[sig_idx] = m_agg_func[sig_idx](child_sample);
        }

        // Cache the necessary state for descend
        // Per-child circular buffer of doubles that will hold the epoch runtime.
        m_last_sample = in_sample;
        double max_value = 0.0;
        for (auto const &it : in_sample) {
           if (max_value < it[M_SAMPLE_EPOCH_RUNTIME]) {
               max_value = it[M_SAMPLE_EPOCH_RUNTIME];
           }
        }
        m_epoch_runtime_buf->insert(max_value);

        return true;
    }

    void BalancingAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (isnan(in_policy[0])) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): policy is NAN.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (m_last_power_budget != in_policy[0] || m_sample_count == 0) {
            m_last_power_budget = in_policy[0];
            double num_pkg = m_control_idx.size();
            double dram_power = m_platform_io.sample(m_pio_idx[M_PLAT_SAMPLE_DRAM_POWER]);
            // Check that we have enough samples (two) to measure DRAM power
            if (isnan(dram_power)) {
                dram_power = 0.0;
            }
            double target_pkg_power = (m_last_power_budget - dram_power) / num_pkg;
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, target_pkg_power);
            }
        }
        m_sample_count++;
        if (m_sample_count == m_samples_per_control) {
            m_sample_count = 0;
        }
    }

    bool BalancingAgent::sample_platform(std::vector<double> &out_sample)
    {
        bool result = false;
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("BalancingAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (m_update_count == m_updates_per_sample) {
            for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SAMPLE; ++sample_idx) {
                m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
            }

            out_sample[M_SAMPLE_EPOCH_RUNTIME] = m_sample[M_PLAT_SAMPLE_EPOCH_RUNTIME];
            out_sample[M_SAMPLE_POWER] = m_sample[M_PLAT_SAMPLE_PKG_POWER] + m_sample[M_PLAT_SAMPLE_DRAM_POWER]; // Sum of all PKG and DRAM power.
            out_sample[M_SAMPLE_IS_CONVERGED] = m_is_converged;

            m_update_count = 0;
            result = true;
        }
        else {
            ++m_update_count;
        }
        return result;
    }

    void BalancingAgent::wait()
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

    /// @todo Implement this with the refactor of descend()
    void BalancingAgent::split_budget(double total_power_budget,
                                      const std::vector<double> &power_used,
                                      const std::vector<double> &runtime,
                                      std::vector<double> &result)
    {
#ifdef GEOPM_DEBUG
        if (power_used.size() != runtime.size() ||
            power_used.size() != result.size()) {
            throw Exception("BalancingAgent::split_budget(): input vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

    // total_power_budget comes from parent
    // Vectors will be sized to represent power_used and runtime of children.

    }

    std::vector<std::pair<std::string, std::string> > BalancingAgent::report_header(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > BalancingAgent::report_node(void)
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > BalancingAgent::report_region(void)
    {
        return {};
    }

    std::vector<std::string> BalancingAgent::trace_names(void) const
    {
        std::vector<std::string> trace_names;

        trace_names.push_back("epoch_runtime");
        trace_names.push_back("power_package");
        trace_names.push_back("power_dram");
        trace_names.push_back("is_converged");
        trace_names.push_back("power_budget");

        return trace_names;
    }

    void BalancingAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_sample[M_PLAT_SAMPLE_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_PKG_POWER] = m_sample[M_PLAT_SAMPLE_PKG_POWER];
        values[M_TRACE_SAMPLE_DRAM_POWER] = m_sample[M_PLAT_SAMPLE_DRAM_POWER];
        values[M_TRACE_SAMPLE_IS_CONVERGED] = m_is_converged;
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget;
    }

    std::string BalancingAgent::plugin_name(void)
    {
        return "balancer";
    }

    std::unique_ptr<IAgent> BalancingAgent::make_plugin(void)
    {
        return geopm::make_unique<BalancingAgent>();
    }

    std::vector<std::string> BalancingAgent::policy_names(void)
    {
        return {"POWER"};
    }

    std::vector<std::string> BalancingAgent::sample_names(void)
    {
        return {"EPOCH_RUNTIME", "POWER", "IS_CONVERGED"};
    }

    void BalancingAgent::runtime_ratio_calc(int offset,
                                            double mean_child_runtime,
                                            const std::vector<std::pair<double, int> > &child_runtime,
                                            std::vector<double> &epoch_runtime_ratio,
                                            double &ratio_total,
                                            double &median_epoch_runtime)
    {
        int num_children = epoch_runtime_ratio.size();
        ratio_total = 0.0;
        for (int sorted_child_idx = offset; sorted_child_idx < num_children; ++sorted_child_idx) {
            int child_idx = child_runtime[sorted_child_idx].second;
            double curr_target = m_last_child_policy[child_idx][M_POLICY_POWER];
            double last_ratio = curr_target / m_last_power_budget;
            median_epoch_runtime = IPlatformIO::agg_median(m_epoch_runtime_buf->make_vector());
            epoch_runtime_ratio[child_idx] = last_ratio *
                                             (mean_child_runtime * m_magic + median_epoch_runtime) /
                                             (mean_child_runtime * num_children);
            ratio_total += epoch_runtime_ratio[child_idx];
        }
    }
}
