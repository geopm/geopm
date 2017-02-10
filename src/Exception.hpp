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
#ifndef EXCEPTION_HPP_INCLUDE
#define EXCEPTION_HPP_INCLUDE

#include <string>
#include <system_error>
#include "geopm_error.h"


extern "C"
{
    /// @brief Unlinks all shared memory keys in case of error.
    ///
    /// Looks in /dev/shm for any keys starting with "/geopm-shm" or
    /// the GEOPM_SHMKEY environment variable if set.  All matching
    /// keys are unlinked with shm_unlink().
    void geopm_error_destroy_shmem(void);
}

namespace geopm
{
    /// @brief Handle a thrown exception and return an error value.
    ///
    /// This exception handler is used by every geopm C interface to
    /// handle any exceptions that are thrown during execution of a
    /// C++ implementation.  If geopm has been configured with
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
    /// @return Error number, positive numbers are system errors,
    ///         negative numbers are geopm errors.
    int exception_handler(std::exception_ptr eptr);

    /// @brief Class for all geopm specific exceptions.
    ///
    /// All exceptions explicitly thrown by the geopm library will be
    /// of this type.  It derives from std::runtime_error and adds one
    /// method called err_value() which returns the error code
    /// associated with the exception.  There are a number of
    /// different constructors.
    class Exception: public std::runtime_error
    {
        public:
            /// @brief Empty constructor.
            ///
            /// Uses errno to determine the error code.  Enables an
            /// abbreviated what() result.  If errno is zero then
            /// GEOPM_ERROR_RUNTIME (-1) is used for the error code.
            Exception();
            Exception(const Exception &other);
            /// @brief Error number only constructor.
            ///
            /// User provides error code.  Enables an abbreviated
            /// what() result.
            ///
            /// @param [in] err Error code, positive values are system
            ///        errors (see errno(3)), negative values are
            ///        geopm errors.  If zero is specified
            ///        GEOPM_ERROR_RUNTIME (-1) is assumed.
            Exception(int err);
            /// @brief Message and error number constructor.
            ///
            /// User provides message and error code.  The error
            /// message provided is appended to the abbreviated what()
            /// result.
            ///
            /// @param [in] what Extension to error message returned
            ///        by what() method.
            ///
            /// @param [in] err Error code, positive values are system
            ///        errors (see errno(3)), negative values are
            ///        geopm errors.  If zero is specified
            ///        GEOPM_ERROR_RUNTIME (-1) is assumed.
            Exception(const std::string &what, int err);
            /// @brief Error number and line number constructor.
            ///
            /// User provides error code, file name, and line number.
            /// The what() method produces the abbreviated message
            /// with the file and line number information appended.
            ///
            /// @param [in] err Error code, positive values are system
            ///        errors (see errno(3)), negative values are
            ///        geopm errors.  If zero is specified
            ///        GEOPM_ERROR_RUNTIME (-1) is assumed.
            ///
            /// @param [in] file Name of source file where exception
            ///        was thrown, e.g. preprocessor __FILE__.
            ///
            /// @param [in] line Line number in source file where
            ///        exception was thrown, e.g. preprocessor
            ///        __LINE__.
            Exception(int err, const char *file, int line);
            /// @brief Message, error number, file and line
            ///        constructor.
            ///
            /// User provides message, error cede, file name and line
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
            ///        geopm errors.  If zero is specified
            ///        GEOPM_ERROR_RUNTIME (-1) is assumed.
            ///
            /// @param [in] file Name of source file where exception
            ///        was thrown, e.g. preprocessor __FILE__.
            ///
            /// @param [in] line Line number in source file where
            ///        exception was thrown, e.g. preprocessor
            ///        __LINE__.
            Exception(const std::string &what, int err, const char *file, int line);
            /// @brief Exception destructor, virtual.
            virtual ~Exception();
            /// @brief Returns the integer error code associated with
            ///        the exception.
            ///
            /// Returns the non-zero error code associated with the
            /// exception.  Negative error codes are geopm specific
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

    class SignalException: public Exception
    {
        public:
            SignalException();
            SignalException(int signum);
            SignalException(const SignalException &other);
            virtual ~SignalException();
            int sig_value(void) const;
        private:
            int m_sig;
    };

}

#endif
