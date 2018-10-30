/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <fstream>
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    std::string read_file(const std::string& path)
    {
        std::string contents;
        std::ifstream input_file(path, std::ifstream::in);
        if (!input_file.is_open()) {
            throw Exception("Helper::" + std::string(__func__) + "(): file \"" + path +
                            "\" could not be opened", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        input_file.seekg(0, std::ios::end);
        size_t file_size = input_file.tellg();
        if (file_size <= 0) {
            throw Exception("Helper::" + std::string(__func__) + "(): input file invalid",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        contents.resize(file_size);
        input_file.seekg(0, std::ios::beg);
        input_file.read(&contents[0], file_size);
        input_file.close();

        return contents;
    }
}

