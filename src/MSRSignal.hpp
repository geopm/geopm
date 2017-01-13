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

#ifndef MSRSIGNAL_HPP_INCLUDE
#define MSRSIGNAL_HPP_INCLUDE

#include <sys/types.h>
#include "Signal.hpp"

namespace geopm
{
    class MSRSignal : public Signal
    {
        public:
            MSRSignal(std::vector<off_t> offset, int num_source);
            virtual ~MSRSignal();
            virtual void decode(const std::vector<uint64_t> &encoded, std::vector<double> &decoded);
            int num_source(void) const;
            int num_encoded(void) const;
            void offset(std::vector<uint64_t> off) const;
            void num_bit(int encoded_idx, int size);
            void left_shift(int encoded_idx, int shift_size);
            void right_shift(int encoded_idx, int shift_size);
            void mask(int encoded_idx, uint64_t bit_mask);
            void scalar(int encoded_idx, double scalar);
        protected:
            int m_num_source;
            std::vector<off_t> m_offset;
            std::vector<int> m_lshift;
            std::vector<int> m_rshift;
            std::vector<uint64_t> m_mask;
            std::vector<double> m_scalar;
            std::vector<int> m_num_bit;
            std::vector<uint64_t> m_raw_last;
            std::vector<uint64_t> m_overflow_offset;
            uint64_t m_raw_value_last;
            off_t m_msr_overflow_offset;
    };

#if 0
    void example_use_sketch(void)
    {
        /* Belongs inside of the platform imp*/
        const std::map<std::string, struct geopm_msr_encode_s> &emap = msr_encode_map(void);
        uint64_t num_batch = 0;

        auto it = emap.find("PERF_CTR0");
        if (it = emap.end()) {
            throw Exception();
        }
        struct geopm_msr_encode_s estruct = (*it).second;
        std::vector<MSRSignal> signal_vec;
        signal_vec.push_back(MSRSignal(2, plat_imp->num_domain(GEOPM_DOMAIN_SIGNAL_PERF)));
        signal_vec.back().num_bit(0, estruct.num_bit);
        signal_vec.back().left_shift(0, estruct.lshift);
        num_batch += signal_vec.back().num_source() * signal_vec.back().num_encoded();
        ...

        it = emap.find("PERF_CTR1");
        if (it = emap.end()) {
            throw Exception();
        }
        estruct = (*it).second;
        signal_vec.push_back(MSRSignal(1, plat_imp->num_domain(GEOPM_DOMAIN_SIGNAL_PERF)));
        signal_vec.back().num_bit(1, estruct.num_bit);
        signal_vec.back().left_shift(1, estruct.lshift);
        num_batch += signal_vec.back().num_source() * signal_vec.back().num_encoded();
        ...

        std::vector<uint64_t> signal_off;
        std::vector<int> signal_cpu;
        for (auto it = signal_vec.begin; it != signal_vec.end(); ++it) {
            std::vector<uint64_t> encoded_off;
            (*it).offset(encoded_off);
            for (int dom_idx = 0; dom_idx != (*it).num_source(); ++dom_idx) {
                for (auto off_it = encoded_off.begin(); off_it != encoded_off.end(); ++off_it) {
                    signal_off.push_back((*off_it));
                    signal_cpu.push_back(dom_idx);
                }
            }
        }

        /* Belongs in the PlatformImp */
        MSRAccess access(plat_imp->encode_map(), plat_imp->topo);
        access.config_batch_read(signal_cpu, signal_off);
        std::vector<uint64_t> encode_value(num_batch);
        std::vector<uint64_t> decode_value(num_batch);
        access.read_batch(encode_value);

        encode_it = encode_value.begin();
        decode_it = decode_value.begin();
        for (auto it = signal_vec.begin; it != signal_vec.end(); ++it) {
            for (int i = 0; i !+ (*it).num_source(); ++i) {
                for (int j = 0; j != (*it).num_encoded(); ++j) {
                    *decode_it = (*it).sample(*encode_it);
                    ++encode_it;
                    ++decode_it;
                }
            }
        }
    }
#endif
}

#endif
