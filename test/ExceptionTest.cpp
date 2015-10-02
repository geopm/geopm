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

#include "errno.h"
#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"

class ExceptionTest: public :: testing :: Test {};

TEST_F(ExceptionTest, hello)
{
    geopm::Exception ex0(GEOPM_ERROR_RUNTIME);
    std::cerr << "ERROR: " << ex0.what() << std::endl;
    geopm::Exception ex1("Hello world", GEOPM_ERROR_LOGIC);
    std::cerr << "ERROR: " << ex1.what() << std::endl;
    geopm::Exception ex2(GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    std::cerr << "ERROR: " << ex2.what() << std::endl;
    geopm::Exception ex3("Hello world", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    std::cerr << "ERROR: " << ex3.what() << std::endl;
    std::cerr << "Error value = " << ex3.err_value() << std::endl;
    try {
        throw ex3;
    }
    catch (...) {
        int err = geopm::exception_handler(std::current_exception());
        std::cerr << "Error number = " << err << std::endl;
    }
}

TEST_F(ExceptionTest, hello_invalid)
{
    geopm::Exception ex("Hello world EINVAL error", EINVAL);
    std::cerr << "ERROR: " << ex.what() << std::endl;
}

TEST_F(ExceptionTest, file_info)
{
    geopm::Exception ex("With file info", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    std::cerr << "ERROR: " << ex.what() << std::endl;
    geopm::Exception ex2("", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
    std::cerr << "ERROR: " << ex2.what() << std::endl;
}
