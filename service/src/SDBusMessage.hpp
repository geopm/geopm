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
    /// @brief Abstraction around sd_bus interfaces that read from or
    ///        append to sd_bus_message types.
    ///
    /// A mock-able C++ interface wrapper around the sd_bus functions
    /// that read or modify messages.  To read messages methods can
    /// enter or exit containers, as well as reading strings, doubles
    /// and integers.  The abstraction also enables appending a list
    /// of strings to a container to write to the message.
    ///
    class SDBusMessage
    {
        public:
            /// @brief Used with enter_container() method to specify a
            ///        structure container.
            static const char M_MESSAGE_TYPE_STRUCT;
            /// @brief Used with enter_container() method to specify an
            ///        array container.
            static const char M_MESSAGE_TYPE_ARRAY;
            SDBusMessage() = default;
            virtual ~SDBusMessage() = default;
            /// @brief Factory method for SDBusMessage interface
            ///
            /// @return Unique pointer to an implementation of the
            ///         SDBusMessage interface.
            static std::unique_ptr<SDBusMessage> make_unique(sd_bus_message *bus_message);
            /// @brief Get raw pointer to sd_bus struct.
            ///
            /// This value can be used to make sd_bus interface calls
            /// that require the raw pointer in the SDBus class.
            ///
            /// @return Pointer used to construct the object.
            virtual sd_bus_message *get_sd_ptr(void) = 0;
            /// @brief Enter a container in the message for reading
            ///
            /// Wrapper around sd_bus_message_enter_container(3)
            /// function.
            ///
            /// @param type [in] One of M_MESSAGE_TYPE_STRUCT or
            ///             M_MESSAGE_TYPE_ARRAY which map to the
            ///             related char values defined in "sd-bus.h".
            /// @param contents [in] The sd_bus string expression that
            ///                 describes the data types in the message.
            virtual void enter_container(
                char type,
                const std::string &contents) = 0;
            /// @brief Exit a container in the message for reading
            ///
            /// Wrapper around sd_bus_message_exit_container(3)
            /// function.
            virtual void exit_container(void) = 0;
            /// @brief Open a container in the message for writing
            ///
            /// Wrapper around sd_bus_message_open_container(3)
            ///
            /// @param type [in] One of M_MESSAGE_TYPE_STRUCT or
            ///             M_MESSAGE_TYPE_ARRAY which map to the
            ///             related char values defined in "sd-bus.h".
            /// @param contents [in] The sd_bus string expression that
            ///                 describes the data types in the message.
            virtual void open_container(
                char type,
                const std::string &contents) = 0;
            /// @brief Close a container in the message for writing
            ///
            /// Wrapper around sd_bus_message_close_open_container(3)
            ///
            virtual void close_container(void) = 0;
            /// @brief Read a string out of the message
            ///
            /// Wrapper around the "sd_bus_message_read(3)" function.
            ///
            /// @return The string that is next in the message.
            virtual std::string read_string(void) = 0;
            /// @brief Read a double out of the message
            ///
            /// Wrapper around the "sd_bus_message_read(3)" function.
            ///
            /// @return The double that is next in the message.
            virtual double read_double(void) = 0;
            /// @brief Read a double out of the message
            ///
            /// Wrapper around the "sd_bus_message_read(3)" function.
            ///
            /// @return The double that is next in the message.
            virtual int read_integer(void) = 0;
            /// @brief Write an array of strings into the message
            ///
            /// Wrapper around the "sd_bus_message_append_strv(3)"
            /// function.
            ///
            /// @param [in] Vector of strings to write into the
            ///        message as an array.
            virtual void append_strings(
                const std::vector<std::string> &write_values) = 0;
            /// @brief Determine if end of array has been reached.
            ///
            /// When iterating through an array container, the
            /// was_success() method can be used to determine if the
            /// last read from the container was successful.  After a
            /// read from an array was successful the was_success()
            /// will return true, and if the end of the array has been
            /// reached, was_success() will return false.  The return
            /// value from a read from an array container that was
            /// unsuccessful should be ignored.
            ///
            /// @return Will return true if last read from object was
            ///         successful and false otherwise.
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
            virtual void open_container(
                char type,
                const std::string &contents) override;
            virtual void close_container(void) override;
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
