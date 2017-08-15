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

#include <stdint.h>

enum geopm_profile_e {
    GEOPM_MAX_NUM_CPU = 768
};

/// @brief Structure intended to be shared between
/// the geopm runtime and the application in
/// order to convey status and control information.
struct geopm_ctl_message_s {
    /// @brief Status of the geopm runtime.
    volatile uint32_t ctl_status;
    /// @brief Status of the application.
    volatile uint32_t app_status;
    /// @brief Holds affinities of all application ranks
    /// on the local compute node.
    int cpu_rank[GEOPM_MAX_NUM_CPU];
};

namespace geopm
{
    class IControlMessage
    {
        public:
            IControlMessage() {}
            virtual ~IControlMessage() {}
            virtual void step() = 0;
            virtual void wait() = 0;
            virtual void cpu_rank(int cpu_idx, int rank) = 0;
            virtual int cpu_rank(int cpu_idx) = 0;
            virtual bool is_sample_begin(void) = 0;
            virtual bool is_sample_end(void) = 0;
            virtual bool is_name_begin(void) = 0;
            virtual bool is_shutdown(void) = 0;
            virtual void loop_begin(void) = 0;
    };

    class ControlMessage : public IControlMessage
    {
        public:
            ControlMessage(struct geopm_ctl_message_s *ctl_msg, bool is_ctl, bool is_writer);
            virtual ~ControlMessage();
            void step();
            void wait();
            void cpu_rank(int cpu_idx, int rank);
            int cpu_rank(int cpu_idx);
            bool is_sample_begin(void);
            bool is_sample_end(void);
            bool is_name_begin(void);
            bool is_shutdown(void);
            void loop_begin(void);
        private:
            int this_status();
            /// @brief Enum encompassing application and
            /// geopm runtime state.
            enum m_status_e {
                M_STATUS_UNDEFINED = 0,
                M_STATUS_MAP_BEGIN = 1,
                M_STATUS_MAP_END = 2,
                M_STATUS_SAMPLE_BEGIN = 3,
                M_STATUS_SAMPLE_END = 4,
                M_STATUS_NAME_BEGIN = 5,
                M_STATUS_NAME_LOOP_BEGIN = 6,
                M_STATUS_NAME_LOOP_END = 7,
                M_STATUS_NAME_END = 8,
                M_STATUS_SHUTDOWN = 9,
            };
            struct geopm_ctl_message_s *m_ctl_msg;
            bool m_is_ctl;
            bool m_is_writer;
            int m_last_status;

    };

}
