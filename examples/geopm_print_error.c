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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "geopm_error.h"
#ifndef NAME_MAX
#define NAME_MAX 512
#endif

int main(int argc, char **argv)
{
    int return_code = 0;

    const int error_codes[] = {
        GEOPM_ERROR_RUNTIME,
        GEOPM_ERROR_LOGIC,
        GEOPM_ERROR_INVALID,
        GEOPM_ERROR_POLICY_NULL,
        GEOPM_ERROR_FILE_PARSE,
        GEOPM_ERROR_LEVEL_RANGE,
        GEOPM_ERROR_CTL_COMM,
        GEOPM_ERROR_SAMPLE_INCOMPLETE,
        GEOPM_ERROR_POLICY_UNKNOWN,
        GEOPM_ERROR_NOT_IMPLEMENTED,
        GEOPM_ERROR_NOT_TESTED,
        GEOPM_ERROR_PLATFORM_UNSUPPORTED,
        GEOPM_ERROR_MSR_OPEN,
        GEOPM_ERROR_MSR_READ,
        GEOPM_ERROR_MSR_WRITE,
        GEOPM_ERROR_OPENMP_UNSUPPORTED,
        GEOPM_ERROR_PROF_NULL,
        GEOPM_ERROR_DECIDER_UNSUPPORTED,
        GEOPM_ERROR_FACTORY_NULL,
        GEOPM_ERROR_SHUTDOWN,
        GEOPM_ERROR_TOO_MANY_COLLISIONS,
        GEOPM_ERROR_AFFINITY,
        GEOPM_ERROR_ENVIRONMENT,
        GEOPM_ERROR_COMM_UNSUPPORTED,
    };

    const char *error_names[] = {
        "GEOPM_ERROR_RUNTIME",
        "GEOPM_ERROR_LOGIC",
        "GEOPM_ERROR_INVALID",
        "GEOPM_ERROR_POLICY_NULL",
        "GEOPM_ERROR_FILE_PARSE",
        "GEOPM_ERROR_LEVEL_RANGE",
        "GEOPM_ERROR_CTL_COMM",
        "GEOPM_ERROR_SAMPLE_INCOMPLETE",
        "GEOPM_ERROR_POLICY_UNKNOWN",
        "GEOPM_ERROR_NOT_IMPLEMENTED",
        "GEOPM_ERROR_NOT_TESTED",
        "GEOPM_ERROR_PLATFORM_UNSUPPORTED",
        "GEOPM_ERROR_MSR_OPEN",
        "GEOPM_ERROR_MSR_READ",
        "GEOPM_ERROR_MSR_WRITE",
        "GEOPM_ERROR_OPENMP_UNSUPPORTED",
        "GEOPM_ERROR_PROF_NULL",
        "GEOPM_ERROR_DECIDER_UNSUPPORTED",
        "GEOPM_ERROR_FACTORY_NULL",
        "GEOPM_ERROR_SHUTDOWN",
        "GEOPM_ERROR_TOO_MANY_COLLISIONS",
        "GEOPM_ERROR_AFFINITY",
        "GEOPM_ERROR_ENVIRONMENT",
        "GEOPM_ERROR_COMM_UNSUPPORTED",
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
    char message[NAME_MAX];
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
        geopm_error_message(error_codes[i], message, NAME_MAX);
        message[NAME_MAX - 1] = '\0';
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
