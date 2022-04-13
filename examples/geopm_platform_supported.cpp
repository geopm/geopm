/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "geopm/PlatformTopo.hpp"
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
