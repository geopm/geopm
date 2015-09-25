/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef PLATFORMIMP_HPP_INCLUDE
#define PLATFORMIMP_HPP_INCLUDE

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include "PlatformTopology.hpp"

namespace geopm
{

    /* Platform IDs
    ((family << 8) + model)
    0x62A - Sandy Bridge
    0x62D - Sandy Bridge E
    0x63A - Ivy Bridge
    0x63E - Ivy Bridge E
    0x63C - Haswell
    0x645 - Haswell
    0x646 - Haswell
    0x63f - Haswell E
    */

    class PlatformImp
    {
        public:
            PlatformImp();
            virtual ~PlatformImp();

            //////////////////////////
            // Topology Information //
            //////////////////////////
            uint32_t get_num_package(void) const;
            uint32_t get_num_tile(void) const;
            uint32_t get_num_cpu(void) const;
            uint32_t get_num_hyperthreads(void) const;
            PlatformTopology topology(void) const;

            ///////////////////////////
            // MSR read/write support //
            ///////////////////////////
            void write_msr(int device_type, int device_index, const std::string &msr_name, uint64_t value);
            void write_msr(int device_type, int device_index, off_t msr_offset, uint64_t value);
            uint64_t read_msr(int device_type, int device_index, const std::string &msr_name);
            uint64_t read_msr(int device_type, int device_index, off_t msr_offset);
            off_t get_msr_offset(std::string msr_name);
            virtual void set_msr_path(int cpu_num);

            ////////////////////////////////////////
            // Platform dependent implementations //
            ////////////////////////////////////////
            virtual bool model_supported(int platform_id) = 0;
            virtual std::string get_platform_name(void) = 0;
            virtual void initialize_msrs() = 0;
            virtual void reset_msrs(void) = 0;

        protected:
            void open_msr(int cpu);
            void close_msr(int cpu);
            virtual void parse_hw_topology(void);
            PlatformTopology m_topology;
            std::vector<int> m_cpu_file_descs;
            std::map<std::string, off_t> m_msr_offset_map;
            int m_hyperthreads;
            int m_num_cpu;
            int m_num_tile;
            int m_num_package;
            char m_msr_path[NAME_MAX];
    };
}

#endif
