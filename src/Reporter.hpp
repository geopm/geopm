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

#ifndef REPORTER_HPP_INCLUDE
#define REPORTER_HPP_INCLUDE

#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <memory>
#include <vector>

#include "geopm_time.h"

namespace geopm
{
    class IComm;
    class IApplicationIO;
    class IPlatformIO;
    class ITreeComm;

    class IReporter
    {
        public:
            IReporter() = default;
            virtual ~IReporter() = default;
            virtual void generate(const std::string &agent_name,
                                  const std::string &agent_report_header,
                                  const std::string &agent_node_report,
                                  const std::map<uint64_t, std::string> &agent_region_report,
                                  const IApplicationIO &application_io,
                                  std::shared_ptr<IComm> comm,
                                  const ITreeComm &tree_comm) = 0;
    };

    class Reporter : public IReporter
    {
        public:
            Reporter(const std::string &report_name, IPlatformIO &platform_io);
            virtual ~Reporter() = default;
            void generate(const std::string &agent_name,
                          const std::string &agent_report_header,
                          const std::string &agent_node_report,
                          const std::map<uint64_t, std::string> &agent_region_report,
                          const IApplicationIO &application_io,
                          std::shared_ptr<IComm> comm,
                          const ITreeComm &tree_comm) override;
        private:
            std::string get_max_memory(void);

            std::string m_report_name;
            IPlatformIO &m_platform_io;
            int m_energy_idx;
            int m_clk_core_idx;
            int m_clk_ref_idx;
            bool m_is_root;
    };
}

#endif
