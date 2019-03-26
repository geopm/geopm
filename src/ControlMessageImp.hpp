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

#ifndef CONTROLMESSAGEIMP_HPP_INCLUDE
#define CONTROLMESSAGEIMP_HPP_INCLUDE

#include "ControlMessage.hpp"

namespace geopm
{
    class ControlMessageImp : public ControlMessage
    {
        public:
            /// @brief ControlMessageImp constructor.
            ///
            /// @param [in] ctl_msg Buffer allocated in shared memory
            /// that is large enough to store a geopm_ctl_message_s.
            /// The memory should be attached by the controller and
            /// the application.
            ///
            /// @param [in] is_ctl Boolean value that is true if the
            /// caller is the GEOPM controller and false if the caller
            /// is the application under control.
            ///
            /// @param [in] is_writer Boolean value that is true if
            /// the caller is the controller or the lowest application
            /// rank on the node and false if the caller is any other
            /// application rank.
            ControlMessageImp(struct geopm_ctl_message_s &ctl_msg, bool is_ctl, bool is_writer);
            /// @brief ControlMessageImp virtual destructor
            virtual ~ControlMessageImp() = default;
            void step() override;
            void wait() override;
            void abort(void) override;
            void cpu_rank(int cpu_idx, int rank) override;
            int cpu_rank(int cpu_idx) const override;
            bool is_sample_begin(void) const override;
            bool is_sample_end(void) const override;
            bool is_name_begin(void) const override;
            bool is_shutdown(void) const override;
            void loop_begin(void) override;
            /// @brief Enum encompassing application and
            /// GEOPM runtime state.
            enum m_status_e {
                M_STATUS_UNDEFINED,
                M_STATUS_MAP_BEGIN,
                M_STATUS_MAP_END,
                M_STATUS_SAMPLE_BEGIN,
                M_STATUS_SAMPLE_END,
                M_STATUS_NAME_BEGIN,
                M_STATUS_NAME_LOOP_BEGIN,
                M_STATUS_NAME_LOOP_END,
                M_STATUS_NAME_END,
                M_STATUS_SHUTDOWN,
                M_STATUS_ABORT = 9999,
            };
        private:
            int this_status() const;
            const double M_WAIT_SEC;
            struct geopm_ctl_message_s &m_ctl_msg;
            bool m_is_ctl;
            bool m_is_writer;
            int m_last_status;
    };
}
#endif
