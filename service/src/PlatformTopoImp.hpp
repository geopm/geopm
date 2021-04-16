/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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


#ifndef PLATFORMTOPOIMP_HPP_INCLUDE
#define PLATFORMTOPOIMP_HPP_INCLUDE

#include "PlatformTopo.hpp"

namespace geopm
{
    class AcceleratorTopo;

    class PlatformTopoImp : public PlatformTopo
    {
        public:
            PlatformTopoImp();
            PlatformTopoImp(const std::string &test_cache_file_name);
            PlatformTopoImp(const std::string &test_cache_file_name, const AcceleratorTopo &accelerator_topo);
            virtual ~PlatformTopoImp() = default;
            int num_domain(int domain_type) const override;
            int domain_idx(int domain_type,
                           int cpu_idx) const override;
            bool is_nested_domain(int inner_domain, int outer_domain) const override;
            std::set<int> domain_nested(int inner_domain, int outer_domain, int outer_idx) const override;
            static void create_cache();
            static void create_cache(const std::string &cache_file_name);
        private:
            static const std::string M_CACHE_FILE_NAME;
            /// @brief Get the set of Linux logical CPUs associated
            ///        with the indexed domain.
            std::set<int> domain_cpus(int domain_type,
                                      int domain_idx) const;

            void lscpu(std::map<std::string, std::string> &lscpu_map);
            void parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                             int &num_package,
                             int &core_per_package,
                             int &thread_per_core);
            void parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
                                  std::vector<std::set<int> > &numa_map);
            FILE *open_lscpu(void);
            void close_lscpu(FILE *fid);

            const std::string M_TEST_CACHE_FILE_NAME;
            bool m_do_fclose;
            int m_num_package;
            int m_core_per_package;
            int m_thread_per_core;
            std::vector<std::set<int> > m_numa_map;
            const AcceleratorTopo &m_accelerator_topo;
    };
}
#endif
