/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AGENT_HPP_INCLUDE
#define AGENT_HPP_INCLUDE

#include <string>
#include <map>
#include <vector>
#include <functional>

#include "geopm/PluginFactory.hpp"

namespace geopm
{
    class Agent
    {
        public:
            Agent() = default;
            virtual ~Agent() = default;
            static std::vector<std::string> agent_names(void);
            static std::unique_ptr<Agent> make_unique(const std::string &agent_name);
            /// @brief Set the level where this Agent is active and push
            ///        signals/controls for that level.
            /// @param [in] level Level of the tree where this agent
            ///        is active.  Note that only agents at level zero
            ///        execute sample_platform() and adjust_platform().
            /// @param [in] fan_in Vector over level giving the the
            ///        number of Agents that report to each root Agent
            ///        operating at the level.
            /// @param [in] is_level_root True if the agent plays the
            ///        role of root of the level.  Only root agents
            ///        for a level execute ascend() and descend().
            virtual void init(int level, const std::vector<int> &fan_in, bool is_level_root) = 0;
            /// @brief Called by Controller to validate incoming policy
            ///        values and configure defaults requested in incoming
            ///        policy.  Policy sender can request default value with
            ///        'NaN' at the desired offset in the policy vector.
            ///        Returned policy should not contain 'NaN's and be
            ///        consumeable by descend and adjust_platform.
            /// @param [in, out] policy Policy replace NaN's with defaults.
            ///        If a value of policy is not NaN but the value is not
            ///        supported by the Agent the method will throw a geopm::Exception
            ///        with error code GEOPM_ERROR_INVALID.
            virtual void validate_policy(std::vector<double> &policy) const = 0;
            /// @brief Called by Controller to split policy for
            ///        children at next level down the tree.
            /// @param [in] in_policy Policy values from the parent.
            /// @param [out] out_policy Vector of policies to be sent
            ///        to each child.
            virtual void split_policy(const std::vector<double> &in_policy,
                                      std::vector<std::vector<double> > &out_policy) = 0;
            /// @brief Called by Controller to determine if new policy values
            ///        should be sent down the tree to the Agent's children.
            /// @return True if the policy has been updated since last call.
            virtual bool do_send_policy(void) const = 0;
            /// @brief Aggregate samples from children for the next
            ///        level up the tree.
            /// @param [in] in_sample Vector of sample vectors, one
            ///        from each child.
            /// @param [out] out_sample Aggregated sample values to be
            ///        sent up to the parent.
            virtual void aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                          std::vector<double> &out_sample) = 0;
            /// @return True if new samples were read since last call.
            virtual bool do_send_sample(void) const = 0;
            /// @brief Adjust the platform settings based the policy
            ///        from above.
            /// @param [in] policy Settings for each control in the
            ///        policy.
            virtual void adjust_platform(const std::vector<double> &in_policy) = 0;
            /// @brief Called by the Controller to decide whether to call
            ///        write_batch() to update platform controls.
            /// @return True if platform was adjusted, false otherwise.
            virtual bool do_write_batch(void) const = 0;
            /// @brief Read signals from the platform and
            ///        interpret/aggregate these signals to create a
            ///        sample which can be sent up the tree.
            /// @param [out] sample Vector of agent specific sample
            ///        values to be sent up the tree.
            virtual void sample_platform(std::vector<double> &out_sample) = 0;
            /// @brief Called by Controller to wait for sample period
            ///        to elapse.  This controls the cadence of the
            ///        Controller main loop.
            virtual void wait(void) = 0;
            /// @brief Custom fields that will be added to the report
            ///        header when this agent is used.
            virtual std::vector<std::pair<std::string, std::string> > report_header(void) const = 0;
            /// @brief Custom fields for the host section of the report.
            virtual std::vector<std::pair<std::string, std::string> > report_host(void) const = 0;
            /// @brief Custom fields for each region in the report.
            virtual std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const = 0;
            /// @brief Column headers to be added to the trace.
            virtual std::vector<std::string> trace_names(void) const = 0;
            /// @brief Returns format string for each column added to the trace
            virtual std::vector<std::function<std::string(double)> > trace_formats(void) const;
            /// @brief Called by Controller to get latest values to be
            ///        added to the trace.
            virtual void trace_values(std::vector<double> &values) = 0;
            /// @brief Enforce the policy one time with
            ///        PlatformIO::write_control().  Called to enforce
            ///        static policies in the absence of a Controller.
            virtual void enforce_policy(const std::vector<double> &policy) const = 0;
            /// @brief Used to look up the number of values in the
            ///        policy vector sent down the tree for a specific
            ///        Agent.  This should be called with the
            ///        dictionary returned by
            ///        agent_factory().dictionary(agent_name) for the
            ///        Agent of interest.
            /// @param [in] dictionary Factory dictionary for the agent.
            static int num_policy(const std::map<std::string, std::string> &dictionary);
            /// @brief Used to look up the number of values in the
            ///        policy vector sent down the tree for a specific
            ///        Agent.
            /// @param [in] agent_name Name of the agent.
            static int num_policy(const std::string &agent_name);
            /// @brief Used to look up the number of values in the
            ///        sample vector sent up the tree for a specific
            ///        Agent.  This should be called with the
            ///        dictionary returned by
            ///        agent_factory().dictionary(agent_name) for the
            ///        Agent of interest.
            /// @param [in] dictionary Factory dictionary for the agent.
            static int num_sample(const std::map<std::string, std::string> &dictionary);
            /// @brief Used to look up the number of values in the
            ///        sample vector sent up the tree for a specific
            ///        Agent.
            /// @param [in] agent_name Name of the agent.
            static int num_sample(const std::string &agent_name);
            /// @brief Used to look up the names of values in the
            ///        policy vector sent down the tree for a specific
            ///        Agent.  This should be called with the
            ///        dictionary returned by
            ///        agent_factory().dictionary(agent_name) for the
            ///        Agent of interest.
            /// @param [in] dictionary Factory dictionary for the agent.
            static std::vector<std::string> policy_names(const std::map<std::string, std::string> &dictionary);
            /// @brief Used to look up the names of values in the
            ///        policy vector sent down the tree for a specific
            ///        Agent.
            /// @param [in] agent_name Name of the agent.
            static std::vector<std::string> policy_names(const std::string &agent_name);
            /// @brief Used to look up the names of values in the
            ///        sample vector sent up the tree for a specific
            ///        Agent.  This should be called with the
            ///        dictionary returned by
            ///        agent_factory().dictionary(agent_name) for the
            ///        Agent of interest.
            /// @param [in] dictionary Factory dictionary for the agent.
            static std::vector<std::string> sample_names(const std::map<std::string, std::string> &dictionary);
            /// @brief Used to look up the names of values in the
            ///        sample vector sent up the tree for a specific
            ///        Agent.
            /// @param [in] agent_name Name of the agent.
            static std::vector<std::string> sample_names(const std::string &agent_name);
            /// @brief Used to create a correctly-formatted dictionary
            ///        for an Agent at the time the Agent is
            ///        registered with the factory.  Concrete Agent
            ///        classes may provide policy_names() and
            ///        sample_names() methods to provide the vectors
            ///        to be passed to this method.
            static std::map<std::string, std::string> make_dictionary(const std::vector<std::string> &policy_names,
                                                                      const std::vector<std::string> &sample_names);
            /// @brief Generically aggregate a vector of samples given
            ///        a vector of aggregation functions.  This helper
            ///        method applies a different aggregation
            ///        function to each sample element while
            ///        aggregating across child samples.
            /// @param [in] in_sample Vector over children of the
            ///        sample vector received from each child.
            /// @param [in] agg_func A vector over agent samples of
            ///        the aggregation function that is applied.
            /// @param [out] out_sample Sample vector resulting from
            ///        the applying the aggregation across child
            ///        samples.
            static void aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                         const std::vector<std::function<double(const std::vector<double>&)> > &agg_func,
                                         std::vector<double> &out_sample);
            static const std::string M_PLUGIN_PREFIX;
        private:
            static const std::string m_num_sample_string;
            static const std::string m_num_policy_string;
            static const std::string m_sample_prefix;
            static const std::string m_policy_prefix;
    };

    class AgentFactory : public PluginFactory<Agent>
    {
        public:
            AgentFactory();
            virtual ~AgentFactory() = default;
    };

    AgentFactory &agent_factory(void);
}

#endif
