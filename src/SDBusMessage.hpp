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

#ifndef SDBUSMESSAGE_HPP_INCLUDE
#define SDBUSMESSAGE_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>

struct sd_bus_message;

namespace geopm
{
    class SDBusMessage
    {
        public:
            static const char M_MESSAGE_TYPE_STRUCT;
            static const char M_MESSAGE_TYPE_ARRAY;
            SDBusMessage() = default;
            virtual ~SDBusMessage() = default;
            static std::unique_ptr<SDBusMessage> make_unique(sd_bus_message *bus_message);
            virtual sd_bus_message *get_sd_ptr(void) = 0;
            virtual void enter_container(
                char type,
                const std::string &contents) = 0;
            virtual void exit_container(void) = 0;
            virtual std::string read_string(void) = 0;
            virtual double read_double(void) = 0;
            virtual int read_integer(void) = 0;
            virtual void append_strings(
                const std::vector<std::string> &write_values) = 0;
            virtual bool was_success(void) = 0;
    };

    class SDBusMessageImp : public SDBusMessage
    {
        public:
            SDBusMessageImp();
            SDBusMessageImp(sd_bus_message *bus_message);
            virtual ~SDBusMessageImp() = default;
            sd_bus_message *get_sd_ptr(void) override;
            void enter_container(
                char type,
                const std::string &contents) override;
            void exit_container(void) override;
            std::string read_string(void) override;
            double read_double(void) override;
            int read_integer(void) override;
            void append_strings(
                const std::vector<std::string> &write_values) override;
            bool was_success(void) override;
        private:
            sd_bus_message *m_bus_message;
            bool m_was_success;
    };
}

#endif
