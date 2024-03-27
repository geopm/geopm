/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "geopm_hash.h"

int main(int argc, char **argv)
{
    const char *usage = "%s string\n\n"
                        "    Returns the geopm_crc32_str() of the input string.\n"
                        "    this is useful for determining the region name\n"
                        "    from the region_id printed in the trace.\n";
    if (argc == 1 ||
        strncmp("-h", argv[1], strlen("-h")) == 0 ||
        strncmp("--help", argv[1], strlen("--help")) == 0) {
        printf(usage, argv[0]);
    }
    else {
        printf("0x%.16llx\n", (unsigned long long)geopm_crc32_str(argv[1]));
    }
    return 0;
}
