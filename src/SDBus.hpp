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

#ifndef SDBUS_HPP_INCLUDE
#define SDBUS_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>
#include <systemd/sd-bus.h>

struct sd_bus;

namespace geopm
{
    class SDBusMessage;

    class SDBus
    {
        public:
            SDBus() = default;
            virtual ~SDBus() = default;
            static std::unique_ptr<SDBus> make_unique(void);
            virtual std::shared_ptr<SDBusMessage> call(
                std::shared_ptr<SDBusMessage> message) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2,
                double arg3) = 0;
            virtual std::shared_ptr<SDBusMessage> make_call_message(
                const std::string &member) = 0;
    };

    class SDBusImp : public SDBus
    {
        public:
            SDBusImp();
            virtual ~SDBusImp();
            std::shared_ptr<SDBusMessage> call(
                std::shared_ptr<SDBusMessage> message) override;
            std::shared_ptr<SDBusMessage> call_method(
                const std::string &member) override;
            std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2) override;
            std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2,
                double arg3) override;
            std::shared_ptr<SDBusMessage> make_call_message(
                const std::string &member) override;
        private:
            sd_bus *m_bus;
            const char * m_dbus_destination;
            const char * m_dbus_path;
            const char * m_dbus_interface;
            const uint64_t m_dbus_timeout_usec;
    };
}

#endif
