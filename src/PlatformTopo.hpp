/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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


#ifndef PLATFORMTOPO_HPP_INCLUDE
#define PLATFORMTOPO_HPP_INCLUDE

#include <vector>
#include <set>
#include <map>
#include <string>
#include <stdio.h>

namespace geopm
{
    class IPlatformTopo
    {
        public:
            enum m_domain_e {
                /// @brief Reserved to represent an invalid domain
                M_DOMAIN_INVALID = 0,
                /// @brief All components on a user allocated compute node (one per controller)
                M_DOMAIN_BOARD,
                /// @Brief Single processor package in one socket
                M_DOMAIN_PACKAGE,
                /// @brief Group of associated hyper-threads
                M_DOMAIN_CORE,
                /// @brief Linux logical CPU
                M_DOMAIN_CPU,
                /// @brief Standard off package DIMM (DRAM or NAND)
                M_DOMAIN_BOARD_MEMORY,
                /// @brief On package memory (MCDRAM)
                M_DOMAIN_PACKAGE_MEMORY,
                /// @brief Network interface controller on the PCI bus
                M_DOMAIN_BOARD_NIC,
                /// @brief Network interface controller on the processor package
                M_DOMAIN_PACKAGE_NIC,
                /// @brief Accelerator card on the PCI bus
                M_DOMAIN_BOARD_ACCELERATOR,
                /// @brief Accelerator unit on the package (e.g on-package graphics)
                M_DOMAIN_PACKAGE_ACCELERATOR,
                /// @brief Start of user defined collections of Linux logical CPUs
                M_DOMAIN_CPU_GROUP_BASE = 4096,
            };

            IPlatformTopo() {}
            virtual ~IPlatformTopo() {}
            /// @brief Number of domains on the platform of a
            ///        particular m_domain_e type.
            virtual int num_domain(int domain_type) const = 0;
            /// @brief Get the set of Linux logical CPUs associated with the indexed domain
            virtual void domain_cpus(int domain_type,
                                     int domain_idx,
                                     std::set<int> &cpu_idx) const = 0;
            /// @brief Get the domain index for a particular domain
            ///        type that contains the given Linux logical CPU
            ///        index
            virtual int domain_idx(int domain_type,
                                   int cpu_idx) const = 0;
            /// @brief Define a new domain type that is a group of
            ///        Linux logical CPUs by assigning a domain index
            ///        to each.
            /// @param [in] cpu_domain_idx A vector over Linux logical CPUs
            ///        assigning a domain index to each.
            virtual int define_cpu_group(const std::vector<int> &cpu_domain_idx) = 0;
    };

    IPlatformTopo &platform_topo(void);

    class PlatformTopo : public IPlatformTopo
    {
        public:
            PlatformTopo();
            PlatformTopo(const std::string &lscpu_file_name);
            virtual ~PlatformTopo();
            int num_domain(int domain_type) const;
            void domain_cpus(int domain_type,
                             int domain_idx,
                             std::set<int> &cpu_idx) const;
            int domain_idx(int domain_type,
                           int cpu_idx) const;
            int define_cpu_group(const std::vector<int> &cpu_domain_idx);
        protected:
            void lscpu(std::map<std::string, std::string> &lscpu_map);
            void parse_lscpu(const std::map<std::string, std::string> &lscpu_map,
                             int &num_package,
                             int &core_per_package,
                             int &thread_per_core);
            void parse_lscpu_numa(std::map<std::string, std::string> lscpu_map,
                                  std::vector<std::set<int> > &numa_map);
            FILE *open_lscpu(void);
            void close_lscpu(FILE *fid);

            const std::string m_lscpu_file_name;
            int m_num_package;
            int m_core_per_package;
            int m_thread_per_core;
            std::vector<std::set<int> > m_numa_map;
    };

}
#endif
