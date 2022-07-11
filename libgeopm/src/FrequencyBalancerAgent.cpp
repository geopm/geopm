/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "FrequencyBalancerAgent.hpp"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iterator>
#include <numeric>
#include <thread>
#include <utility>

#include "geopm/Exception.hpp"
#include "geopm/FrequencyGovernor.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PowerGovernor.hpp"
#include "geopm_hash.h"
#include "geopm_hint.h"
#include "geopm_time.h"
#include "geopm_topo.h"

#include "Environment.hpp"
#include "FrequencyLimitDetector.hpp"
#include "FrequencyTimeBalancer.hpp"
#include "SSTClosGovernor.hpp"
#include "Waiter.hpp"
#include "geopm_debug.hpp"

// Minimum number of sampling wait periods before applying new epoch controls.
#define MINIMUM_WAIT_PERIODS_FOR_NEW_EPOCH_CONTROL 5

// Minimum number of epochs to wait before applying new epoch controls.
#define MINIMUM_EPOCHS_FOR_NEW_EPOCH_CONTROL 3

// Number of back-to-back network hints to treat as "in a network region."
// Lower numbers respond more quickly, but risk throttling regions that
// happen to land next to a short-running network region.
// -- Arbitrarily set to 3 because that produces acceptable behavior so far.
#define NETWORK_HINT_MINIMUM_SAMPLE_LENGTH 3

// Number of back-to-back non-network hints to treat as not "in a network region."
#define NON_NETWORK_HINT_MINIMUM_SAMPLE_LENGTH 1

namespace geopm
{
    FrequencyBalancerAgent::FrequencyBalancerAgent()
        : FrequencyBalancerAgent(platform_io(), platform_topo(),
                                 Waiter::make_unique(environment().period(M_WAIT_SEC)),
                                 PowerGovernor::make_shared(),
                                 FrequencyGovernor::make_shared(),
                                 (SSTClosGovernor::is_supported(platform_io())
                                      ? SSTClosGovernor::make_shared()
                                      : nullptr),
                                 {}, {})
    {
    }

    static bool is_all_nan(const std::vector<double> &vec)
    {
        return std::all_of(vec.begin(), vec.end(),
                           [](double x) -> bool { return std::isnan(x); });
    }

    FrequencyBalancerAgent::FrequencyBalancerAgent(PlatformIO &plat_io,
                                                   const PlatformTopo &topo,
                                                   std::shared_ptr<Waiter> waiter,
                                                   std::shared_ptr<PowerGovernor> power_gov,
                                                   std::shared_ptr<FrequencyGovernor> frequency_gov,
                                                   std::shared_ptr<SSTClosGovernor> sst_gov,
                                                   std::vector<std::shared_ptr<FrequencyTimeBalancer> > package_balancers,
                                                   std::shared_ptr<FrequencyLimitDetector> frequency_limit_detector)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_waiter(std::move(waiter))
        , m_update_time(time_zero())
        , m_num_children(0)
        , m_is_policy_updated(false)
        , m_do_write_batch(false)
        , m_is_adjust_initialized(false)
        , m_is_real_policy(false)
        , m_package_count(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , m_package_core_indices()
        , m_policy_power_package_limit_total(NAN)
        , m_policy_use_frequency_limits(true)
        , m_use_sst_tf(false)
        , m_min_power_setting(m_platform_io.read_signal("CPU_POWER_MIN_AVAIL",
                                                        GEOPM_DOMAIN_BOARD, 0))
        , m_max_power_setting(m_platform_io.read_signal("CPU_POWER_MAX_AVAIL",
                                                        GEOPM_DOMAIN_BOARD, 0))
        , m_tdp_power_setting(m_platform_io.read_signal("CPU_POWER_LIMIT_DEFAULT",
                                                        GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_min(
              m_platform_io.read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_sticker(m_platform_io.read_signal("CPU_FREQUENCY_STICKER",
                                                        GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_max(
              m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        , m_frequency_step(
              m_platform_io.read_signal("CPU_FREQUENCY_STEP", GEOPM_DOMAIN_BOARD, 0))
        , m_power_gov(power_gov)
        , m_freq_governor(frequency_gov)
        , m_sst_clos_governor(sst_gov)
        , m_frequency_ctl_domain_type(m_freq_governor->frequency_domain_type())
        , m_frequency_control_domain_count(m_platform_topo.num_domain(m_frequency_ctl_domain_type))
        , m_network_hint_sample_length(m_frequency_control_domain_count, 0)
        , m_non_network_hint_sample_length(m_frequency_control_domain_count, 0)
        , m_last_hp_count(2, 0)
        , m_handle_new_epoch(false)
        , m_epoch_wait_count(MINIMUM_EPOCHS_FOR_NEW_EPOCH_CONTROL)
        , m_package_balancers(package_balancers)
        , m_frequency_limit_detector(frequency_limit_detector)
    {
        if (m_package_balancers.empty()) {
            for (int i = 0; i < m_package_count; ++i) {
                // We balance each CPU package independently so we can take
                // advantage of power headroom within each package. Create
                // one balancer per package.
                m_package_balancers.push_back(FrequencyTimeBalancer::make_unique(
                    m_frequency_min, m_frequency_max, m_frequency_step));
            }
        }
        for (int i = 0; i < m_package_count; ++i) {
            // Determine which control indices (e.g., CPUs or cores) map to
            // each balancer. We need this since some operations (e.g., platform IO)
            // are made with respect to all controls in a domain, and others (e.g.,
            // intra-package rebalancing) operate on subsets.
            auto control_indices_in_package = m_platform_topo.domain_nested(
                m_frequency_ctl_domain_type, GEOPM_DOMAIN_PACKAGE, i);
            m_package_core_indices.emplace_back(
                control_indices_in_package.begin(), control_indices_in_package.end());
        }
        if (!m_frequency_limit_detector) {
            m_frequency_limit_detector = FrequencyLimitDetector::make_unique(
                m_platform_io, m_platform_topo);
        }

        if (m_sst_clos_governor) {
            m_frequency_ctl_domain_type = m_sst_clos_governor->clos_domain_type();
            m_frequency_control_domain_count = m_platform_topo.num_domain(m_frequency_ctl_domain_type);
            m_network_hint_sample_length.resize(m_frequency_control_domain_count, 0);
            m_non_network_hint_sample_length.resize(m_frequency_control_domain_count, 0);
        }
    }

    std::string FrequencyBalancerAgent::plugin_name(void)
    {
        return "frequency_balancer";
    }

    std::unique_ptr<Agent> FrequencyBalancerAgent::make_plugin(void)
    {
        return geopm::make_unique<FrequencyBalancerAgent>();
    }

    void FrequencyBalancerAgent::init(int level, const std::vector<int> &fan_in,
                                     bool is_level_root)
    {
        static_cast<void>(is_level_root);
        if (level == 0) {
            m_num_children = 0;
            init_platform_io();
        }
        else {
            m_num_children = fan_in[level - 1];
        }

        m_platform_io.write_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0, m_frequency_max);
    }

    void FrequencyBalancerAgent::validate_policy(std::vector<double> &policy) const
    {
        GEOPM_DEBUG_ASSERT(policy.size() == M_NUM_POLICY,
                           "FrequencyBalancerAgent::validate_policy(): "
                           "policy vector not correctly sized.");

        if (std::isnan(policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL])) {
            policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = m_tdp_power_setting;
        }
        if (policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] < m_min_power_setting) {
            policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = m_min_power_setting;
        }
        else if (policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] > m_max_power_setting) {
            policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = m_max_power_setting;
        }

        if (std::isnan(policy[M_POLICY_USE_FREQUENCY_LIMITS])) {
            policy[M_POLICY_USE_FREQUENCY_LIMITS] = 1;
        }

        if (std::isnan(policy[M_POLICY_USE_SST_TF])) {
            policy[M_POLICY_USE_SST_TF] = 1;
        }

        if (policy[M_POLICY_USE_FREQUENCY_LIMITS] == 0 && policy[M_POLICY_USE_SST_TF] == 0) {
            throw Exception("FrequencyBalancerAgent::validate_policy(): must allow at "
                            "least one of frequency limits or SST-TF.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void FrequencyBalancerAgent::update_policy(const std::vector<double> &policy)
    {
        if (is_all_nan(policy) && !m_is_real_policy) {
            // All-NAN policy is ignored until first real policy is received
            m_is_policy_updated = false;
            return;
        }
        m_is_real_policy = true;
    }

    void FrequencyBalancerAgent::split_policy(const std::vector<double> &in_policy,
                                             std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t)m_num_children) {
            throw Exception("FrequencyBalancerAgent::" + std::string(__func__) +
                                "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        for (auto &child_policy : out_policy) {
            if (child_policy.size() != M_NUM_POLICY) {
                throw Exception("FrequencyBalancerAgent::" + std::string(__func__) +
                                    "(): child_policy vector not correctly sized.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
#endif
        update_policy(in_policy);

        if (m_is_policy_updated) {
            std::fill(out_policy.begin(), out_policy.end(), in_policy);
        }
    }

    bool FrequencyBalancerAgent::do_send_policy(void) const
    {
        return m_is_policy_updated;
    }

    void FrequencyBalancerAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                                 std::vector<double> &out_sample)
    {
        static_cast<void>(in_sample);
        static_cast<void>(out_sample);
    }

    bool FrequencyBalancerAgent::do_send_sample(void) const
    {
        return false;
    }

    void FrequencyBalancerAgent::initialize_policies(const std::vector<double> &in_policy)
    {
        double power_budget_in = in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL];
        m_policy_power_package_limit_total = power_budget_in;
        double adjusted_power;
        m_power_gov->adjust_platform(power_budget_in, adjusted_power);
        m_do_write_batch = true;

        m_policy_use_frequency_limits = in_policy[M_POLICY_USE_FREQUENCY_LIMITS] != 0;
        bool sst_tf_is_supported = m_sst_clos_governor && SSTClosGovernor::is_supported(m_platform_io);
        m_use_sst_tf = in_policy[M_POLICY_USE_SST_TF] != 0 && sst_tf_is_supported;

        m_freq_governor->set_frequency_bounds(m_frequency_min, m_frequency_max);

        // Initialize to max frequency limit and max CLOS
        m_freq_governor->adjust_platform(m_last_ctl_frequency);
        if (sst_tf_is_supported) {
            if (m_use_sst_tf) {
                m_sst_clos_governor->adjust_platform(m_last_ctl_clos);
                m_sst_clos_governor->enable_sst_turbo_prioritization();
            } else {
                m_sst_clos_governor->disable_sst_turbo_prioritization();
            }
        }
    }

    void FrequencyBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        update_policy(in_policy);

        m_do_write_batch = false;
        if (!m_is_adjust_initialized) {
            initialize_policies(in_policy);
            m_is_adjust_initialized = true;
            return;
        }

        std::vector<double> frequency_by_core = m_last_ctl_frequency;
        std::vector<double> clos_by_core = m_last_ctl_clos;

        if (m_handle_new_epoch) {
            m_handle_new_epoch = false;
            m_frequency_limit_detector->update_max_frequency_estimates(
                m_last_epoch_max_frequency);
            for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
                std::vector<double> pkg_ctl_frequency;
                pkg_ctl_frequency.reserve(m_package_core_indices[package_idx].size());

                double max_achievable_frequency = 0;
                std::vector<std::pair<unsigned int, double> > core_frequency_limits;
                double low_priority_frequency = m_frequency_min;

                for (auto ctl_idx : m_package_core_indices[package_idx]) {
                    pkg_ctl_frequency.push_back(m_last_ctl_frequency[ctl_idx]);
                    auto limits = m_frequency_limit_detector->get_core_frequency_limits(ctl_idx);
                    // Assume that the max-achievable frequency is the greatest
                    // of all expected achievable frequencies on app cores in this package
                    for (const auto &limit : limits) {
                        if (limit.second > max_achievable_frequency) {
                            core_frequency_limits = limits;
                            max_achievable_frequency = limit.second;
                            low_priority_frequency = m_frequency_limit_detector->get_core_low_priority_frequency(ctl_idx);
                        }
                    }
                }
                frequency_by_core = m_package_balancers[package_idx]->balance_frequencies_by_time(
                    m_last_epoch_non_network_time_diff[package_idx],
                    pkg_ctl_frequency,
                    m_last_epoch_frequency[package_idx],
                    core_frequency_limits,
                    low_priority_frequency);

                for (size_t pkg_nested_ctl_idx = 0; pkg_nested_ctl_idx < m_package_core_indices[package_idx].size(); ++pkg_nested_ctl_idx) {
                    // Note: ctl_idx is the index used for PlatformIO interactions.
                    //       pkg_nested_ctl_idx is the local index within per-package vectors.
                    auto ctl_idx = m_package_core_indices[package_idx][pkg_nested_ctl_idx];
                    if (m_last_ctl_frequency[ctl_idx] > frequency_by_core[pkg_nested_ctl_idx]) {
                        // Going down. Round up toward previous.
                        frequency_by_core[pkg_nested_ctl_idx] = std::ceil(frequency_by_core[pkg_nested_ctl_idx] / m_frequency_step) * m_frequency_step;
                    }
                    else {
                        // Going up. Round down toward previous.
                        frequency_by_core[pkg_nested_ctl_idx] = std::floor(frequency_by_core[pkg_nested_ctl_idx] / m_frequency_step) * m_frequency_step;
                    }
                    m_last_ctl_frequency[ctl_idx] = frequency_by_core[pkg_nested_ctl_idx];
                }
            }
            geopm_time(&m_update_time);
        }

        // Apply immediate controls for workloads that change rapidly within
        // epochs.
        auto immediate_ctl_frequency = m_last_ctl_frequency;
        for (size_t package_idx = 0;
             package_idx < static_cast<size_t>(m_package_count); ++package_idx) {
            int hp_not_waiting_count = 0;
            for (size_t pkg_nested_ctl_idx = 0; pkg_nested_ctl_idx < m_package_core_indices[package_idx].size(); ++pkg_nested_ctl_idx) {
                // Note: ctl_idx is the index used for PlatformIO interactions.
                //       pkg_nested_ctl_idx is the local index within per-package vectors.
                const auto ctl_idx = m_package_core_indices[package_idx][pkg_nested_ctl_idx];

                if (std::isnan(m_last_hash[ctl_idx]) || m_last_hash[ctl_idx] == GEOPM_REGION_HASH_INVALID) {
                    // Non-application regions get the expected low-priority
                    // frequency so we can focus our turbo budget on
                    // application regions.
                    immediate_ctl_frequency[ctl_idx] = m_frequency_limit_detector->get_core_low_priority_frequency(ctl_idx);
                }
                else if(m_network_hint_sample_length[ctl_idx] >= NETWORK_HINT_MINIMUM_SAMPLE_LENGTH) {
                    // Don't assume that (last hint)==NETWORK alone implies
                    // that we are in a network region because it may be a
                    // short-lasting region that we just happened to sample.
                    immediate_ctl_frequency[ctl_idx] = m_frequency_limit_detector->get_core_low_priority_frequency(ctl_idx);
                } else if (immediate_ctl_frequency[ctl_idx] >= m_frequency_max) {
                    // This is a HP core that is not in a networking region
                    hp_not_waiting_count += 1;
                }
            }

            if (hp_not_waiting_count == 0) {
                // All HP cores are waiting... Move the all non-waiting cores to HP
                for (size_t pkg_nested_ctl_idx = 0; pkg_nested_ctl_idx < m_package_core_indices[package_idx].size(); ++pkg_nested_ctl_idx) {
                    // Note: ctl_idx is the index used for PlatformIO interactions.
                    //       pkg_nested_ctl_idx is the local index within per-package vectors.
                    const auto ctl_idx = m_package_core_indices[package_idx][pkg_nested_ctl_idx];
                    if (m_non_network_hint_sample_length[ctl_idx] >= NON_NETWORK_HINT_MINIMUM_SAMPLE_LENGTH) {
                        immediate_ctl_frequency[ctl_idx] = m_frequency_max;
                    }
                }
            }
        }

        if (m_use_sst_tf) {
            // Assign to low priority CLOS if we expect that the core's
            // workload properties allow it to achieve our requested frequency
            // while in the low priority CLOS configuration. Otherwise, set it
            // to high priority CLOS.
            // The reason for selecting priority this way is that we are taking
            // a conservative approach to risks of over-throttling a core vs
            // missed opportunities from underthrottling cores.
            for (size_t ctl_idx = 0; ctl_idx < frequency_by_core.size(); ++ctl_idx) {
                clos_by_core[ctl_idx] =
                    (immediate_ctl_frequency[ctl_idx] > m_frequency_limit_detector->get_core_low_priority_frequency(ctl_idx))
                        ? SSTClosGovernor::HIGH_PRIORITY
                        : SSTClosGovernor::LOW_PRIORITY;
            }
            m_sst_clos_governor->adjust_platform(clos_by_core);
        }
        if (m_policy_use_frequency_limits) {
            m_freq_governor->adjust_platform(immediate_ctl_frequency);
        }
    }


    bool FrequencyBalancerAgent::do_write_batch(void) const
    {
        bool do_write = m_do_write_batch || m_freq_governor->do_write_batch();
        if (m_use_sst_tf) {
            do_write |= m_sst_clos_governor->do_write_batch();
        }
        return do_write;
    }

    void FrequencyBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        static_cast<void>(out_sample);

        for (size_t ctl_idx = 0;
             ctl_idx < (size_t)m_frequency_control_domain_count; ++ctl_idx) {
            m_last_hash[ctl_idx] = m_platform_io.sample(m_hash_signal_idx[ctl_idx]);
            auto last_hint = m_platform_io.sample(m_hint_signal_idx[ctl_idx]);
            if (last_hint == GEOPM_REGION_HINT_NETWORK) {
                m_network_hint_sample_length[ctl_idx] += 1;
                m_non_network_hint_sample_length[ctl_idx] = 0;
            }
            else {
                m_network_hint_sample_length[ctl_idx] = 0;
                m_non_network_hint_sample_length[ctl_idx] += 1;
            }

            const auto prev_sample_acnt = m_last_sample_acnt[ctl_idx];
            const auto prev_sample_mcnt = m_last_sample_mcnt[ctl_idx];
            m_last_sample_acnt[ctl_idx] = m_platform_io.sample(m_acnt_signal_idx[ctl_idx]);
            m_last_sample_mcnt[ctl_idx] = m_platform_io.sample(m_mcnt_signal_idx[ctl_idx]);
            const auto last_sample_frequency =
                (m_last_sample_acnt[ctl_idx] - prev_sample_acnt) /
                (m_last_sample_mcnt[ctl_idx] - prev_sample_mcnt) * m_frequency_sticker;
            if (!std::isnan(m_last_hash[ctl_idx]) && m_last_hash[ctl_idx] != GEOPM_REGION_HASH_INVALID) {
                m_current_epoch_max_frequency[ctl_idx] = std::max(m_current_epoch_max_frequency[ctl_idx], last_sample_frequency);
            }
        }

        double epoch_count = m_platform_io.sample(m_epoch_signal_idx);
        const auto counted_epochs = epoch_count - m_last_epoch_count;
        if (!std::isnan(epoch_count) && !std::isnan(m_last_epoch_count) &&
            counted_epochs >= m_epoch_wait_count) {
            double new_epoch_time = m_platform_io.read_signal(
                "TIME", GEOPM_DOMAIN_BOARD, 0);
            auto last_epoch_time_diff = new_epoch_time - m_last_epoch_time;
            if (last_epoch_time_diff < MINIMUM_WAIT_PERIODS_FOR_NEW_EPOCH_CONTROL * m_waiter->period()) {
                // Wait for some number of epochs to pass before calculating
                // time per epoch. The wait period should end at the boundary of
                // an epoch, but the minimum wait length depends on our sample
                // rate. We are trying to reduce the impact of aliasing on the
                // TIME_HINT_NETWORK signal.
                m_epoch_wait_count += 1;
            }
            else {
                // We encountered a new epoch and we are ready to evaluate the
                // signal differences over the previous group of epochs.
                for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
                    for (size_t pkg_nested_ctl_idx = 0; pkg_nested_ctl_idx < m_package_core_indices[package_idx].size(); ++pkg_nested_ctl_idx) {
                        // Note: ctl_idx is the index used for PlatformIO interactions.
                        //       pkg_nested_ctl_idx is the local index within per-package vectors.
                        const auto ctl_idx = m_package_core_indices[package_idx][pkg_nested_ctl_idx];
                        const auto prev_epoch_acnt = m_last_epoch_acnt[ctl_idx];
                        const auto prev_epoch_mcnt = m_last_epoch_mcnt[ctl_idx];
                        m_last_epoch_acnt[ctl_idx] =
                            m_platform_io.sample(m_acnt_signal_idx[ctl_idx]);
                        m_last_epoch_mcnt[ctl_idx] =
                            m_platform_io.sample(m_mcnt_signal_idx[ctl_idx]);
                        m_last_epoch_frequency[package_idx][pkg_nested_ctl_idx] =
                            (m_last_epoch_acnt[ctl_idx] - prev_epoch_acnt) /
                            (m_last_epoch_mcnt[ctl_idx] - prev_epoch_mcnt) * m_frequency_sticker;

                        double new_epoch_network_time =
                            m_platform_io.sample(m_time_hint_network_idx[ctl_idx]);
                        auto last_epoch_network_time_diff =
                            new_epoch_network_time - m_last_epoch_network_time[package_idx][pkg_nested_ctl_idx];
                        m_last_epoch_non_network_time_diff[package_idx][pkg_nested_ctl_idx] =
                            std::max(0.0, last_epoch_time_diff - last_epoch_network_time_diff) /
                            counted_epochs;
                        m_last_epoch_network_time[package_idx][pkg_nested_ctl_idx] = new_epoch_network_time;
                    }
                }
                m_last_epoch_max_frequency.swap(m_current_epoch_max_frequency);
                std::fill(m_current_epoch_max_frequency.begin(),
                          m_current_epoch_max_frequency.end(),
                          m_frequency_min);

                m_last_epoch_time = new_epoch_time;
                m_last_epoch_count = epoch_count;
                m_epoch_wait_count = MINIMUM_EPOCHS_FOR_NEW_EPOCH_CONTROL;
                m_handle_new_epoch = true;
            }
        }
        else if (std::isnan(m_last_epoch_count)) {
            // We don't want to update the previous epoch count if the wait
            // period has not been exceeded. BUT we must update it if the
            // previous count was NaN (e.g., we are observing the first epoch
            // now)
            m_last_epoch_count = epoch_count;
        }
    }

    void FrequencyBalancerAgent::wait(void)
    {
        m_waiter->wait();
    }

    std::vector<std::string> FrequencyBalancerAgent::policy_names(void)
    {
        return { "POWER_PACKAGE_LIMIT_TOTAL",
                 "USE_FREQUENCY_LIMITS", "USE_SST_TF" };
    }

    std::vector<std::string> FrequencyBalancerAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> >
        FrequencyBalancerAgent::report_header(void) const
    {
        return {
            {"Agent uses frequency control", std::to_string(m_policy_use_frequency_limits)},
            {"Agent uses SST-TF", std::to_string(m_use_sst_tf)},
        };
    }

    std::vector<std::pair<std::string, std::string> >
        FrequencyBalancerAgent::report_host(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >
        FrequencyBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> FrequencyBalancerAgent::trace_names(void) const
    {
        std::vector<std::string> names;
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            for (auto core_idx : m_package_core_indices[package_idx]) {
                names.push_back(std::string("NON_NET_TIME_PER_EPOCH-core-") + std::to_string(core_idx));
            }
        }
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            names.push_back(std::string("DESIRED_NON_NETWORK_TIME-package-") + std::to_string(package_idx));
        }
        return names;
    }

    std::vector<std::function<std::string(double)> >
        FrequencyBalancerAgent::trace_formats(void) const
    {
        std::vector<std::function<std::string(double)> > formats;
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            for (size_t i = 0; i < m_package_core_indices[package_idx].size(); ++i) {
                formats.push_back(string_format_double);
            }
        }
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            formats.push_back(string_format_double);
        }
        return formats;
    }

    void FrequencyBalancerAgent::trace_values(std::vector<double> &values)
    {
        values.clear();
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            values.insert(values.end(),
                          m_last_epoch_non_network_time_diff[package_idx].begin(),
                          m_last_epoch_non_network_time_diff[package_idx].end());
        }
        for (const auto &balancer : m_package_balancers) {
            values.push_back(balancer->get_target_time());
        }
    }

    void FrequencyBalancerAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("FrequencyBalancerAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void FrequencyBalancerAgent::init_platform_io(void)
    {
        m_power_gov->init_platform_io();
        m_freq_governor->set_domain_type(m_frequency_ctl_domain_type);
        m_freq_governor->init_platform_io();
        if (SSTClosGovernor::is_supported(m_platform_io)) {
            m_sst_clos_governor->init_platform_io();
        }
        m_last_ctl_frequency = std::vector<double>(
            m_frequency_control_domain_count, m_frequency_max);
        m_last_ctl_clos = std::vector<double>(
            m_frequency_control_domain_count, SSTClosGovernor::HIGH_PRIORITY);
        m_last_epoch_acnt = std::vector<double>(m_frequency_control_domain_count, NAN);
        m_last_epoch_mcnt = std::vector<double>(m_frequency_control_domain_count, NAN);
        m_last_sample_acnt = std::vector<double>(m_frequency_control_domain_count, NAN);
        m_last_sample_mcnt = std::vector<double>(m_frequency_control_domain_count, NAN);
        m_last_hash = std::vector<double>(m_frequency_control_domain_count, NAN);
        m_current_epoch_max_frequency = std::vector<double>(m_frequency_control_domain_count, m_frequency_min);
        m_last_epoch_max_frequency = std::vector<double>(m_frequency_control_domain_count, NAN);
        for (int package_idx = 0; package_idx < m_package_count; ++package_idx) {
            m_last_epoch_frequency.push_back(std::vector<double>(m_package_core_indices[package_idx].size(), NAN));
            m_last_epoch_network_time.push_back(std::vector<double>(m_package_core_indices[package_idx].size(), NAN));
            m_last_epoch_non_network_time_diff.push_back(std::vector<double>(m_package_core_indices[package_idx].size(), NAN));
        }
        m_last_epoch_count = NAN;
        m_last_epoch_time = NAN;
        for (size_t ctl_idx = 0;
             ctl_idx < (size_t)m_frequency_control_domain_count; ++ctl_idx) {
            m_acnt_signal_idx.push_back(m_platform_io.push_signal(
                "MSR::APERF:ACNT", m_frequency_ctl_domain_type, ctl_idx));
            m_mcnt_signal_idx.push_back(m_platform_io.push_signal(
                "MSR::MPERF:MCNT", m_frequency_ctl_domain_type, ctl_idx));
            m_hash_signal_idx.push_back(m_platform_io.push_signal(
                "REGION_HASH", m_frequency_ctl_domain_type, ctl_idx));
            m_hint_signal_idx.push_back(m_platform_io.push_signal(
                "REGION_HINT", m_frequency_ctl_domain_type, ctl_idx));
            m_time_hint_network_idx.push_back(m_platform_io.push_signal(
                "TIME_HINT_NETWORK", m_frequency_ctl_domain_type, ctl_idx));
        }
        m_epoch_signal_idx = m_platform_io.push_signal("EPOCH_COUNT",
                                                       GEOPM_DOMAIN_BOARD, 0);
    }
}
