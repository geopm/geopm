/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "geopm_error.h"

int main(int argc, char **argv)
{
    int return_code = 0;

    const int error_codes[] = {
        GEOPM_ERROR_RUNTIME,
        GEOPM_ERROR_LOGIC,
        GEOPM_ERROR_INVALID,
        GEOPM_ERROR_FILE_PARSE,
        GEOPM_ERROR_LEVEL_RANGE,
        GEOPM_ERROR_NOT_IMPLEMENTED,
        GEOPM_ERROR_PLATFORM_UNSUPPORTED,
        GEOPM_ERROR_MSR_OPEN,
        GEOPM_ERROR_MSR_READ,
        GEOPM_ERROR_MSR_WRITE,
        GEOPM_ERROR_AGENT_UNSUPPORTED,
        GEOPM_ERROR_AFFINITY,
        GEOPM_ERROR_NO_AGENT,
        GEOPM_ERROR_DATA_STORE,
    };

    const char *error_names[] = {
        "GEOPM_ERROR_RUNTIME",
        "GEOPM_ERROR_LOGIC",
        "GEOPM_ERROR_INVALID",
        "GEOPM_ERROR_FILE_PARSE",
        "GEOPM_ERROR_LEVEL_RANGE",
        "GEOPM_ERROR_NOT_IMPLEMENTED",
        "GEOPM_ERROR_PLATFORM_UNSUPPORTED",
        "GEOPM_ERROR_MSR_OPEN",
        "GEOPM_ERROR_MSR_READ",
        "GEOPM_ERROR_MSR_WRITE",
        "GEOPM_ERROR_AGENT_UNSUPPORTED",
        "GEOPM_ERROR_AFFINITY",
        "GEOPM_ERROR_NO_AGENT",
        "GEOPM_ERROR_DATA_STORE",
    };

    enum format_type_e {
        GEOPM_FORMAT_TYPE_HUMAN,
        GEOPM_FORMAT_TYPE_ROFF,
        GEOPM_FORMAT_TYPE_RONN
    };

    const int num_error = sizeof(error_codes) / sizeof(int);
    const char *format_human = "    %s = %i\n        %s\n";
    const char *format_roff = ".TP\n.B %s = %i\n%s\n";
    const char *format_ronn = "  * `%s = %i`:\n    %s\n\n";

    const char *tag = "<geopm> ";
    int format_type = GEOPM_FORMAT_TYPE_HUMAN;
    const char *usage = "%s [--help] [--roff]\n";
    char message[PATH_MAX];
    int i;

    if (argc == 2 && strncmp(argv[1], "--roff", strlen("--roff")) == 0 ) {
        format_type = GEOPM_FORMAT_TYPE_ROFF;
    }
    else if (argc == 2 && strncmp(argv[1], "--ronn", strlen("--ronn")) == 0 ) {
        format_type = GEOPM_FORMAT_TYPE_RONN;
    }
    else if (argc == 2 && (strncmp(argv[1], "--help", strlen("--help")) == 0 ||
                           strncmp(argv[1], "-h", strlen("-h")) == 0)) {
        printf(usage, argv[0]);
        return 0;
    }
    else if (argc != 1) {
        printf(usage, argv[0]);
        fprintf(stderr, "Error: Invalid command line\n");
        return EINVAL;
    }

    if (format_type == GEOPM_FORMAT_TYPE_HUMAN) {
        printf("GEOPM ERROR CODES\n");
    }
    for (i = 0; !return_code && i < num_error; ++i) {
        geopm_error_message(error_codes[i], message, PATH_MAX);
        message[PATH_MAX - 1] = '\0';
        if (strstr(message, tag) == message) {
            switch (format_type) {
                case GEOPM_FORMAT_TYPE_HUMAN:
                    printf(format_human, error_names[i], error_codes[i], message + strlen(tag));
                    break;
                case GEOPM_FORMAT_TYPE_ROFF:
                    printf(format_roff, error_names[i], error_codes[i], message + strlen(tag));
                    break;
                case GEOPM_FORMAT_TYPE_RONN:
                    printf(format_ronn, error_names[i], error_codes[i], message + strlen(tag));
                    break;
            }
        }
        else {
            fprintf(stderr, "Error: <%s> Message does not begin with the tag \"%s\"\n", argv[0], tag);
            return_code = -1;
        }
        if (error_codes[i] >= 0) {
            fprintf(stderr, "Error: <%s> Value for geopm error code is non-negative\n", argv[0]);
            return_code = -2;
        }
        if (strstr(message, "<geopm> Unknown error:") == message ||
            strstr(message, "Unknown error") == message) {
            fprintf(stderr, "Error: <%s> Message has not been implemented for error code.\n", argv[0]);
            return_code = -3;
        }
    }
    printf("\n");
    return return_code;
}
