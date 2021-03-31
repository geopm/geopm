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

#ifndef MODELREGION_HPP_INCLUDE
#define MODELREGION_HPP_INCLUDE

#include <string>
#include <cstdint>
#include <memory>

namespace geopm
{
    class ModelRegion
    {
        public:
            static std::unique_ptr<ModelRegion> model_region(const std::string &name,
                                                             double big_o,
                                                             int verbosity);
            ModelRegion(int verbosity);
            virtual ~ModelRegion();
            std::string name(void);
            double big_o(void);
            virtual int region(void);
            virtual int region(uint64_t hint);
            virtual void region_enter(void);
            virtual void region_exit(void);
            virtual void loop_enter(uint64_t iteration);
            virtual void loop_exit(void);
            virtual void big_o(double big_o_in) = 0;
            virtual void run(void) = 0;
        protected:
            virtual void num_progress_updates(double big_o_in);
            static bool name_check(const std::string &name, const std::string &key);
            std::string m_name;
            double m_big_o;
            int m_verbosity;
            uint64_t m_region_id;
            bool m_do_imbalance;
            bool m_do_progress;
            bool m_do_unmarked;
            uint64_t m_num_progress_updates;
            double m_norm;
    };
}

#endif
