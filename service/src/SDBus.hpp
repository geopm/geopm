/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SDBUS_HPP_INCLUDE
#define SDBUS_HPP_INCLUDE

#include <memory>
#include <string>
#include <vector>

struct sd_bus;

namespace geopm
{
    class SDBusMessage;

    /// @brief Abstraction around sd_bus interface for calling methods
    ///
    /// A mock-able C++ interface wrapper around the sd_bus functions
    /// that initiate calls to GEOPM D-Bus methods.  The sd_bus
    /// functions are provided by the libsystemd.so library and
    /// defined int the systemd/sd-bus.h" header file.  The messages
    /// passed to and from these calls are abstracted by the
    /// SDBusMessage interface.  The SDBus method syntax reflects the
    /// syntax of the underlying sd_bus interface, and for further
    /// information look at the sd-bus(3) man page and links therein.
    class SDBus
    {
        public:
            SDBus() = default;
            virtual ~SDBus() = default;
            /// @brief Factory method for SDBus interface
            ///
            /// @return Unique pointer to an implementation of the
            ///         SDBus interface.
            static std::unique_ptr<SDBus> make_unique(void);
            /// @brief Wrapper for the sd_bus_call(3) function.
            ///
            /// Used to execute a GEOPM D-Bus API using an
            /// SDBusMessage created by the make_call_message()
            /// method.  This enables the user to update the message
            /// with complex data types like arrays and structs to
            /// pass into the method.
            ///
            /// One use case is to to pass lists of strings as inputs
            /// to GEOPM D-Bus APIs.  The SDBusMessage that is created
            /// by make_call_message() is updated using
            /// SDBusMessage::append_strings() prior to passing the
            /// SDBusMessage to the call() method.
            ///
            /// @param message [in] Complete SDBusMessage returned by
            ///                call to make_call_message().
            /// @return Reply message that resulted from the call.
            virtual std::shared_ptr<SDBusMessage> call_method(
                std::shared_ptr<SDBusMessage> message) = 0;
            /// @brief Wrapper for the sd_bus_call_method(3) function.
            ///
            /// Used to execute a GEOPM D-Bus API that takes no
            /// arguments.
            ///
            /// @param member [in] Name of the API from the
            ///               io.github.geopm interface.
            /// @return Reply message that resulted from the call.
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member) = 0;
            /// @brief Wrapper for the sd_bus_call_method(3) function.
            ///
            /// Used to execute a GEOPM D-Bus API that takes three
            /// arguments with types (string, integer, integer).
            ///
            /// @param member [in] Name of the API from the
            ///               io.github.geopm interface.
            /// @param arg0 [in] First parameter to pass to the D-Bus
            ///             API.
            /// @param arg1 [in] Second parameter to pass to the D-Bus
            ///             API.
            /// @param arg2 [in] Third parameter to pass to the D-Bus
            ///             API.
            /// @return Reply message that resulted from the call.
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2) = 0;
            /// @brief Wrapper for the sd_bus_call_method(3) function.
            ///
            /// Used to execute a GEOPM D-Bus API that takes four
            /// arguments with types (string, integer, integer, double).
            ///
            /// @param member [in] Name of the API from the
            ///               io.github.geopm interface.
            /// @param arg0 [in] First parameter to pass to the D-Bus
            ///             API.
            /// @param arg1 [in] Second parameter to pass to the D-Bus
            ///             API.
            /// @param arg2 [in] Third parameter to pass to the D-Bus
            ///             API.
            /// @param arg2 [in] Fourth parameter to pass to the D-Bus
            ///             API.
            /// @return Reply message that resulted from the call.
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0,
                int arg1,
                int arg2,
                double arg3) = 0;
            /// @brief Wrapper for the sd_bus_call_method(3) function.
            ///
            /// Used to execute a GEOPM D-Bus API that takes one
            /// integer argument.
            ///
            /// @param member [in] Name of the API from the
            ///               io.github.geopm interface.
            /// @param arg0 [in] First parameter to pass to the D-Bus
            ///             API.
            /// @return Reply message that resulted from the call.
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                int arg0) = 0;
            /// @brief Wrapper for the
            ///        sd_bus_message_new_method_call(3) function
            ///
            /// This is used to create a SDBusMessage that can be
            /// passed to the call() method.  The user can append data
            /// to the message prior to passing the result to the
            /// call() method in order to send complex data types like
            /// arrays and structures.  The D-Bus API that will be
            /// called later is a parameter to this function.
            ///
            /// @param member [in] Name of the API from the
            ///               io.github.geopm interface.
            /// @return Complete message that can be passed to the
            ///         call() method.
            virtual std::shared_ptr<SDBusMessage> make_call_message(
                const std::string &member) = 0;

            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0) = 0;

    };

    class SDBusImp : public SDBus
    {
        public:
            SDBusImp();
            virtual ~SDBusImp();
            std::shared_ptr<SDBusMessage> call_method(
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
            std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                int arg0) override;
            std::shared_ptr<SDBusMessage> make_call_message(
                const std::string &member) override;
            std::shared_ptr<SDBusMessage> call_method(
                const std::string &member,
                const std::string &arg0) override;
        private:
            sd_bus *m_bus;
            const char *m_dbus_destination;
            const char *m_dbus_path;
            const char *m_dbus_interface;
            const uint64_t m_dbus_timeout_usec;
    };
}

#endif
