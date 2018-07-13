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

#include "PowerGovernorAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerGovernorAgent::PowerGovernorAgent()
        : PowerGovernorAgent(platform_io(), platform_topo())
    {

    }

    PowerGovernorAgent::PowerGovernorAgent(IPlatformIO &platform_io, IPlatformTopo &platform_topo)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_level(-1)
        , m_is_converged(false)
        , m_is_sample_stable(false)
        , m_samples_per_control(10)
        , m_min_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_max_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_policy(M_NUM_POLICY, NAN)
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_agg_func(M_NUM_SAMPLE)
        , m_num_children(0)
        , m_last_power_budget(NAN)
        , m_epoch_power_buf(geopm::make_unique<CircularBuffer<double> >(16)) // Magic number...
        , m_dram_power_buf(geopm::make_unique<CircularBuffer<double> >(16)) // Magic number...
        , m_sample(M_PLAT_NUM_SIGNAL)
        , m_updates_per_sample(5)
        , m_last_energy_status(0.0)
        , m_sample_count(0)
        , m_ascend_count(0)
        , m_ascend_period(10)
        , m_convergence_target(0.01)
        , m_num_out_of_range(0)
        , m_min_num_converged(15)
        , m_num_converged(0)
        , m_num_pkg(m_platform_topo.num_domain(m_platform_io.control_domain_type("POWER_PACKAGE")))
    {

    }

    PowerGovernorAgent::~PowerGovernorAgent()
    {

    }

    void PowerGovernorAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        if (level < 0 || level > (int)fan_in.size()) {
            throw Exception("PowerGovernorAgent::init(): invalid level for given fan_in.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_level = level;
        if (m_level == 0) {
            init_platform_io(); // Only do this at the leaf level.
        }

        if (level == 0) {
            m_num_children = 0;
        }
        else {
            m_num_children = fan_in[level - 1];
        }

        // Setup sample aggregation for data going up the tree
        m_agg_func[M_SAMPLE_POWER] = IPlatformIO::agg_average;
        m_agg_func[M_SAMPLE_IS_CONVERGED] = IPlatformIO::agg_and;
    }

    void PowerGovernorAgent::init_platform_io(void)
    {
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_PKG_POWER] = m_platform_io.push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_DRAM_POWER] = m_platform_io.push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);

        // Setup controls
        int pkg_pwr_domain_type = m_platform_io.control_domain_type("POWER_PACKAGE");
        if (pkg_pwr_domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): Platform does not support package power control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }

        for(int i = 0; i < m_num_pkg; ++i) {
            int control_idx = m_platform_io.push_control("POWER_PACKAGE", pkg_pwr_domain_type, i);
            if (control_idx < 0) {
                throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): Failed to enable package power control"
                                " in the platform.",
                                GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
            }
            m_control_idx.push_back(control_idx);
        }
    }

    bool PowerGovernorAgent::descend(const std::vector<double> &policy_in, std::vector<std::vector<double> > &policy_out)
    {
#ifdef GEOPM_DEBUG
        if (policy_in.size() != M_NUM_POLICY) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): number of policies was different from expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (policy_out.size() != (size_t)m_num_children) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): policy_out vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        bool result = false;
        double power_budget_in = policy_in[M_POLICY_POWER];
        double num_pkg = m_num_pkg;
        double per_package_budget_in = power_budget_in / num_pkg;

        if (per_package_budget_in > m_max_power_setting ||
            per_package_budget_in < m_min_power_setting) {
            throw Exception("PowerGovernorAgent::descend(): "
                            "invalid power budget.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Send down if this is the first budget, or if the budget changed
        if ((std::isnan(m_last_power_budget) && !std::isnan(power_budget_in)) ||
            m_last_power_budget != power_budget_in) {

            m_last_power_budget = power_budget_in;
            // Convert power budget vector into a vector of policy vectors
            for (int child_idx = 0; child_idx != m_num_children; ++child_idx) {
                policy_out[child_idx][M_POLICY_POWER] = power_budget_in;
            }
            m_epoch_power_buf->clear();
            m_dram_power_buf->clear();
            m_is_converged = false;
            result = true;
        }
        return result;
    }

    bool PowerGovernorAgent::ascend(const std::vector<std::vector<double> > &in_sample, std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (in_sample.size() != (size_t)m_num_children) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): in_sample vector not correctly sized.",
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
        // ascend period times, then aggregate the samples and send
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

        return result;
    }

    bool PowerGovernorAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (std::isnan(in_policy[M_POLICY_POWER])) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): policy is NAN.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        bool result = false;
        // If the budget has changed, or we have seen the same budget
        // for m_samples_per_control samples, then update the
        // power limits.
        if (m_last_power_budget != in_policy[M_POLICY_POWER] || m_sample_count == 0) {
            // TODO: sanity check beyond NAN; if DRAM power is too large, target below can go negative
            double dram_power =  IPlatformIO::agg_max(m_dram_power_buf->make_vector());
            // Check that we have enough samples (two) to measure DRAM power
            if (std::isnan(dram_power)) {
                dram_power = 0.0;
            }
            double num_pkg = m_control_idx.size();
            double target_pkg_power = (in_policy[M_POLICY_POWER] - dram_power) / num_pkg;
            if (target_pkg_power < m_min_power_setting) {
                target_pkg_power = m_min_power_setting;
            }
            else if (target_pkg_power > m_max_power_setting) {
                target_pkg_power = m_max_power_setting;
            }
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, target_pkg_power);
            }
            m_last_power_budget = in_policy[M_POLICY_POWER];
            result = true;
        }
        m_sample_count++;
        if (m_sample_count == m_samples_per_control) {
            m_sample_count = 0;
        }
        return result;
    }

    bool PowerGovernorAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerGovernorAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        // Populate sample vector by reading from PlatformIO
        for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SIGNAL; ++sample_idx) {
            m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
        }

        /// @todo should use EPOCH_ENERGY signal which doesn't currently exist
        if (!std::isnan(m_sample[M_PLAT_SIGNAL_PKG_POWER]) && !std::isnan(m_sample[M_PLAT_SIGNAL_DRAM_POWER])) {
            m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_PKG_POWER] + m_sample[M_PLAT_SIGNAL_DRAM_POWER]);
            m_dram_power_buf->insert(m_sample[M_PLAT_SIGNAL_DRAM_POWER]);
        }
        // If we have observed more than m_min_num_converged epoch
        // calls then send median filtered power values up the tree.
        if (m_epoch_power_buf->size() > m_min_num_converged) {
            double median = IPlatformIO::agg_median(m_epoch_power_buf->make_vector());
            out_sample[M_SAMPLE_POWER] = median;
            out_sample[M_SAMPLE_IS_CONVERGED] = (median <= m_last_power_budget); // todo might want fudge factor
            result = true;
        }
        return result;
    }

    void PowerGovernorAgent::wait()
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

    std::vector<std::pair<std::string, std::string> > PowerGovernorAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerGovernorAgent::report_node(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerGovernorAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerGovernorAgent::trace_names(void) const
    {
        return {"power_package",
                "power_dram",
                "power_budget"};
    }

    void PowerGovernorAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("PowerGovernorAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_PKG_POWER] = m_sample[M_PLAT_SIGNAL_PKG_POWER];
        values[M_TRACE_SAMPLE_DRAM_POWER] = m_sample[M_PLAT_SIGNAL_DRAM_POWER];
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget;
    }

    std::string PowerGovernorAgent::plugin_name(void)
    {
        return "power_governor";
    }

    std::unique_ptr<Agent> PowerGovernorAgent::make_plugin(void)
    {
        return geopm::make_unique<PowerGovernorAgent>();
    }

    std::vector<std::string> PowerGovernorAgent::policy_names(void)
    {
        return {"POWER"};
    }

    std::vector<std::string> PowerGovernorAgent::sample_names(void)
    {
        return {"POWER", "IS_CONVERGED"};
    }
}
