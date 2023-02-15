/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef EXCEPTION_HPP_INCLUDE
#define EXCEPTION_HPP_INCLUDE

#include <string>
#include <stdexcept>
#include "geopm_error.h"

namespace geopm
{
    /// @brief Handle a thrown exception and return an error value.
    ///
    /// This exception handler is used by every GEOPM C interface to
    /// handle any exceptions that are thrown during execution of a
    /// C++ implementation.  If GEOPM has been configured with
    /// debugging enabled, then this handler will print an explanatory
    /// message to standard error.  In all cases it will convert the
    /// C++ exception into an error number which can be used with
    /// geopm_error_message() to obtain an error message.  Note that
    /// the error message printed when debugging is enabled has more
    /// specific information than the message produced by
    /// geopm_error_message().
    ///
    /// @param [in] eptr A pointer to a thrown exception such as
    ///        std::current_exception().
    ///
    /// @param [in] do_print A bool specifying whether or not to print
    ///        a debug string to standard error when handling exception.
    ///
    /// @return Error number, positive numbers are system errors,
    ///         negative numbers are GEOPM errors.
    int exception_handler(std::exception_ptr eptr, bool do_print=false);

    /// @brief Class for all GEOPM-specific exceptions.
    ///
    /// All exceptions explicitly thrown by the GEOPM library will be
    /// of this type.  It derives from std::runtime_error and adds one
    /// method called err_value() that returns the error code
    /// associated with the exception.  There are a number of
    /// different constructors.
    class Exception : public std::runtime_error
    {
        public:
            /// @brief Empty constructor.
            ///
            /// Uses errno to determine the error code.  Enables an
            /// abbreviated what() result.  If errno is zero then
            /// GEOPM_ERROR_RUNTIME (-1) is used for the error code.
            Exception();
            Exception(const Exception &other);
            Exception &operator=(const Exception &other);
            /// @brief Message, error number, file and line
            ///        constructor.
            ///
            /// User provides message, error code, file name and line
            /// number.  The what() method appends the user specified
            /// message, file name and line number to the abbreviated
            /// message.  This is the most verbose messaging available
            /// with the Exception class.
            ///
            /// @param [in] what Extension to error message returned
            ///        by what() method.
            ///
            /// @param [in] err Error code, positive values are system
            ///        errors (see errno(3)), negative values are
            ///        GEOPM errors.  If zero is specified
            ///        GEOPM_ERROR_RUNTIME (-1) is assumed.
            ///
            /// @param [in] file Name of source file where exception
            ///        was thrown, e.g. preprocessor `__FILE__`.
            ///
            /// @param [in] line Line number in source file where
            ///        exception was thrown, e.g. preprocessor
            ///        `__LINE__`.
            Exception(const std::string &what, int err, const char *file, int line);
            /// @brief Exception destructor, virtual.
            virtual ~Exception() = default;
            /// @brief Returns the integer error code associated with
            ///        the exception.
            ///
            /// Returns the non-zero error code associated with the
            /// exception.  Negative error codes are GEOPM-specific
            /// and documented in the geopm_error(3) man page.
            /// Positive error codes are system errors and are
            /// documented in the system errno(3) man page.  A brief
            /// description of all error codes can be obtained with
            /// the geopm_error_message(3) interface.
            int err_value(void) const;
        private:
            /// The error code associated with the exception.
            int m_err;
    };

    /// @brief Function that converts an error code into an error message.
    ///
    /// @param error_value The error code associated with the exception.
    ///
    /// @return string The error message associated with the error code.
    std::string error_message(int error_value);
}

#endif
