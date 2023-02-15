/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <limits.h>

#include "geopm_agent.h"
#include "geopm_error.h"

int main(int argc, char *argv[])
{
    int err = geopm_agent_enforce_policy();
    if (err) {
        char err_msg[NAME_MAX];
        geopm_error_message(err, err_msg, NAME_MAX);
        printf("enforce policy failed: %s\n", err_msg);
    }
    return err;
}
