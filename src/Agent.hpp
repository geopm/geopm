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

#ifndef AGENT_HPP_INCLUDE
#define AGENT_HPP_INCLUDE

#include <string>
#include <map>
#include <vector>

#include "PluginFactory.hpp"
#include "PlatformIO.hpp"

namespace geopm
{
    class IAgent
    {
        public:
            IAgent() = default;
            virtual ~IAgent() = default;
            /// Set the level where this Agent is active and push
            /// signals/controls for that level.
            virtual void init(int level) = 0;
            /// Split policy for children at next level down the tree.
            virtual void descend(const std::vector<double> &in_policy,
                                 std::vector<std::vector<double> >&out_policy) = 0;
            /// Aggregate signals from children for the next level up the tree.
            virtual void ascend(const std::vector<std::vector<double> > &in_signal,
                                std::vector<double> &out_signal) = 0;
            /// Adjust the platform settings based the policy from above.
            virtual void adjust_platform(const std::vector<double> &policy) = 0;
            virtual void sample_platform(std::vector<double> &sample) = 0;
            /// Called by Kontroller to wait for sample period to elapse.
            virtual void wait(void) = 0;
            virtual std::vector<std::string> policy_names(void) = 0;
            virtual std::vector<std::string> sample_names(void) = 0;
            virtual std::string report_header(void) = 0;
            virtual std::string report_node(void) = 0;
            virtual std::map<uint64_t, std::string> report_region(void) = 0;
            virtual std::vector<IPlatformIO::m_request_s> trace_columns(void) = 0;
            static int num_sample(const std::map<std::string, std::string> &dictionary);
            static int num_send_down(const std::map<std::string, std::string> &dictionary);
            static std::map<std::string, std::string> make_dictionary(int num_sample, int num_send_down);
        private:
            static const std::string m_num_sample_string;
            static const std::string m_num_policy_string;
    };

    PluginFactory<IAgent> &agent_factory(void);
}

#endif
