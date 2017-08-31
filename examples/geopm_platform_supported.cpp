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

#include <iostream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "PlatformFactory.hpp"
#include "geopm_hash.h"
#include "geopm_error.h"


int main(int argc, char **argv)
{
    int err = 0;
    char error_msg[512];
    error_msg[511] = '\0';

    if (argc == 2 &&
        strncmp(argv[1], "crc32", strlen("crc32") + 1) == 0) {
        uint64_t result = geopm_crc32_u64(0xDEADBEEF, 0xBADFEE);
        if (result == 0xA347ADE3 ) {
            std::cout << "Platform supports crc32 intrinsic." << std::endl;
        }
        else {
            err = GEOPM_ERROR_PLATFORM_UNSUPPORTED;
            std::cerr << "Warning: <geopm_platform_supported>: Platform does not support crc32 intrinsic." << std::endl;
        }
    }
    else {
        int cpu_id = geopm_read_cpuid();
        if (cpu_id != 0x63F &&
            cpu_id != 0x63F &&
            cpu_id != 0x63E &&
            cpu_id != 0x62D &&
            cpu_id != 0x64F &&
            cpu_id != 0x657) {
            err = GEOPM_ERROR_PLATFORM_UNSUPPORTED;
            geopm_error_message(err, error_msg, 511);
            std::cerr << "Warning: <geopm_platform_supported>: Platform 0x" << std::hex << cpu_id << " is not a supported CPU" << " " << error_msg << "." << std::endl;
        }
        if (!err) {
            int fd = open("/dev/cpu/0/msr_safe", O_RDONLY);
            if (fd == -1) {
                err = GEOPM_ERROR_MSR_OPEN;
                geopm_error_message(err, error_msg, 511);
                std::cerr << "Warning: <geopm_platform_supported>: Not able to open msr_safe device. " << error_msg << "." << std::endl;

            }
            else {
                close(fd);
            }
        }
        if (!err) {
           std::cout << "Platform 0x" << std::hex << cpu_id << " is supported by geopm and msr_safe is available." << std::endl;
        }
    }
    return err;
}
