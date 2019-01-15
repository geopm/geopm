/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

extern "C"
{
    /// @brief Identify host CPU.
    int geopm_read_cpuid(void);
}

namespace geopm
{
    class IPlatformTopo
    {
        public:
            enum m_domain_e {
                /// @brief Reserved to represent an invalid domain
                M_DOMAIN_INVALID = 0,
                /// @brief All components on a user allocated compute
                ///        node (one per controller)
                M_DOMAIN_BOARD = 1,
                /// @Brief Single processor package in one socket
                M_DOMAIN_PACKAGE = 2,
                /// @brief Group of associated hyper-threads
                M_DOMAIN_CORE = 3,
                /// @brief Linux logical CPU
                M_DOMAIN_CPU = 4,
                /// @brief Standard off package DIMM (DRAM or NAND)
                M_DOMAIN_BOARD_MEMORY = 5,
                /// @brief On package memory (MCDRAM)
                M_DOMAIN_PACKAGE_MEMORY = 6,
                /// @brief Network interface controller on the PCI bus
                M_DOMAIN_BOARD_NIC = 7,
                /// @brief Network interface controller on the
                ///        processor package
                M_DOMAIN_PACKAGE_NIC = 8,
                /// @brief Accelerator card on the PCI bus
                M_DOMAIN_BOARD_ACCELERATOR = 9,
                /// @brief Accelerator unit on the package (e.g
                ///        on-package graphics)
                M_DOMAIN_PACKAGE_ACCELERATOR = 10,
                M_NUM_DOMAIN,
            };

            IPlatformTopo() = default;
            virtual ~IPlatformTopo() = default;
            /// @brief Number of domains on the platform of a
            ///        particular m_domain_e type.
            virtual int num_domain(int domain_type) const = 0;
            /// @brief Get the domain index for a particular domain
            ///        type that contains the given Linux logical CPU
            ///        index
            virtual int domain_idx(int domain_type,
                                   int cpu_idx) const = 0;
            /// @brief Check if one domain type is contained in another.
            /// @param [in] inner_domain The contained domain type.
            /// @param [in] outer_domain The containing domain type.
            /// @return True if the inner_domain is contained within
            ///         the outer_domain.
            virtual bool is_domain_within(int inner_domain, int outer_domain) const = 0;
            /// @brief Get the set of smaller domains contained in a larger one.
            ///        If the inner domain is not the same as or contained within
            ///        the outer domain, it throws an error.
            /// @param [in] inner_domain The contained domain type.
            /// @param [in] outer_domain The containing domain type.
            /// @param [in] outer_idx The containing domain index.
            /// @return The set of domain indices for the inner domain that are
            ///         within the indexed outer domain.
            virtual std::set<int> nested_domains(int inner_domain, int outer_domain, int outer_idx) const = 0;
            /// @brief Convert a domain type enum to a string.
            /// @param [in] domain_type Domain type from the
            ///        m_domain_e enum.
            /// @return Domain name which is the enum name in
            ///         lowercase with M_DOMAIN_ prefix removed.
            static std::string domain_type_to_name(int domain_type);
            /// @brief Convert a domain name to its corresponding
            ///        enum.
            /// @param [in] name Domain name which is the enum
            ///        in lowercase with M_DOMAIN_ prefix removed.
            /// @return Domain type from the m_domain_e enum.
            static int domain_name_to_type(const std::string &domain_name);
        private:
            static std::vector<std::string> domain_names(void);
            static std::map<std::string, int> domain_types(void);

    };

    IPlatformTopo &platform_topo(void);

    class PlatformTopo : public IPlatformTopo
    {
        public:
            PlatformTopo();
            PlatformTopo(const std::string &lscpu_file_name);
            virtual ~PlatformTopo() = default;
            int num_domain(int domain_type) const override;
            int domain_idx(int domain_type,
                           int cpu_idx) const override;
            bool is_domain_within(int inner_domain, int outer_domain) const override;
            std::set<int> nested_domains(int inner_domain, int outer_domain, int outer_idx) const override;
        private:
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

            const std::string M_LSCPU_FILE_NAME;
            const std::string M_TEST_LSCPU_FILE_NAME;
            bool m_do_fclose;
            int m_num_package;
            int m_core_per_package;
            int m_thread_per_core;
            std::vector<std::set<int> > m_numa_map;
    };

}
#endif
