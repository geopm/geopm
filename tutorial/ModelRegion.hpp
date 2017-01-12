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

#ifndef MODELREGION_HPP_INCLUDE
#define MODELREGION_HPP_INCLUDE

#include <string>
#include <stdint.h>

namespace geopm
{
    class ModelRegionBase
    {
        public:
            ModelRegionBase(int verbosity);
            virtual ~ModelRegionBase();
            std::string name(void);
            double big_o(void);
            virtual void big_o(double big_o_in) = 0;
            virtual void run(void) = 0;
        protected:
            virtual void loop_count(double big_o_in);
            std::string m_name;
            double m_big_o;
            int m_verbosity;
            uint64_t m_region_id;
            bool m_do_imbalance;
            bool m_do_progress;
            uint64_t m_loop_count;
    };

    class SleepModelRegion : public ModelRegionBase
    {
        public:
            SleepModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~SleepModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            struct timespec m_delay;
    };

    class SpinModelRegion : public ModelRegionBase
    {
        friend class NestedModelRegion;
        public:
            SpinModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~SpinModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double m_delay;
    };

    class DGEMMModelRegion : public ModelRegionBase
    {
        public:
            DGEMMModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~DGEMMModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_matrix_a;
            double *m_matrix_b;
            double *m_matrix_c;
            size_t m_matrix_size;
            const size_t m_pad_size;
    };

    class StreamModelRegion : public ModelRegionBase
    {
        public:
            StreamModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~StreamModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            double *m_array_a;
            double *m_array_b;
            double *m_array_c;
            size_t m_array_len;
            const size_t m_align;
    };

    class All2allModelRegion : public ModelRegionBase
    {
        friend class NestedModelRegion;
        public:
            All2allModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~All2allModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            char *m_send_buffer;
            char *m_recv_buffer;
            size_t m_num_send;
            int m_num_rank;
            const size_t m_align;
    };

    class NestedModelRegion : public ModelRegionBase
    {
        public:
            NestedModelRegion(double big_o_in, int verbosity, bool do_imbalance, bool do_progress);
            virtual ~NestedModelRegion();
            void big_o(double big_o);
            void run(void);
        protected:
            SpinModelRegion m_spin_region;
            All2allModelRegion m_all2all_region;
    };

    ModelRegionBase *model_region_factory(std::string name, double big_o, int verbosity);
}

#endif
