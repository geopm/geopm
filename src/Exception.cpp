/*
 * Copyright (c) 2015, Intel Corporation
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

#include <string.h>
#include <stdio.h>

#include "geopm_error.h"
#include "Exception.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

extern "C"
{
    void geopm_error_message(int err, char *msg, size_t size)
    {
        switch (err) {
            case GEOPM_ERROR_RUNTIME:
                strncpy(msg, "<geopm> Runtime error", size);
                break;
            case GEOPM_ERROR_LOGIC:
                strncpy(msg, "<geopm> Logic error", size);
                break;
            case GEOPM_ERROR_INVALID:
                strncpy(msg, "<geopm> Invalid arguement", size);
                break;

            case GEOPM_ERROR_POLICY_NULL:
                strncpy(msg, "<geopm> geopm_policy_c pointer is NULL, use geopm_policy_create", size);
                break;
            case GEOPM_ERROR_FILE_PARSE:
                strncpy(msg, "<geopm> unable to parse input file", size);
                break;
            default:
#ifndef _GNU_SOURCE
                int undef = strerror_r(err, msg, size);
                if (undef && undef != ERANGE) {
                    snprintf(msg, size, "<geopm> Undefined error number: %i", err);
                }
#else
                strncpy(msg, strerror_r(err, msg, size), size);
#endif
                break;
        }
        if (size > 0) {
            msg[size-1] = '\0';
        }
    }
}

namespace geopm
{
    static std::string error_message(int err);

    Exception::Exception(int err)
        : std::runtime_error(error_message(err))
    {}

    Exception::Exception(const std::string &what, int err)
        : std::runtime_error(error_message(err) + ": " + what) 
    {}

    Exception::Exception(const std::string &what, int err, const char *file, int line)
        : std::runtime_error(error_message(err) + ": In file " + std::string(file) + " on line "  + std::to_string(line) + (what.size() == 0 ? "" : ": " + what))
    {}

    Exception::Exception(int err, const char *file, int line)
        : std::runtime_error(error_message(err) + ": In file " + std::string(file) + " on line "  + std::to_string(line))
    {}

    Exception::~Exception()
        {}

    static std::string error_message(int err)
    {
        char tmp_msg[NAME_MAX];
        geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
        return tmp_msg;
    }
}
