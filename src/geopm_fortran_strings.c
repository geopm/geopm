/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015-2017, Intel Corporation
 *
 * Additional copyrights may follow
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "geopm_fortran_strings.h"

/*
 * Free a NULL-terminated argv array.
 */
void geopm_argv_free(char **argv)
{
    char **p;

    if (NULL == argv) {
        return;
    }

    for (p = argv; NULL != *p; ++p) {
        free(*p);
    }

    free(argv);
}

/*
 * Return the length of a NULL-terminated argv array.
 */
int geopm_argv_count(char **argv)
{
    char **p;
    int i;

    if (NULL == argv) {
        return 0;
    }

    for (i = 0, p = argv; *p; i++, p++) {
        continue;
    }

    return i;
}

/*
 * Append a string to the end of a new or existing argv array.
 */
int geopm_argv_append(int *argc, char ***argv, const char *arg)
{
    int rc;

    /* add the new element */
    if (0 != (rc = geopm_argv_append_nosize(argv, arg))) {
        return rc;
    }

    *argc = geopm_argv_count(*argv);

    return 0;
}

int geopm_argv_append_nosize(char ***argv, const char *arg)
{
    int argc;

    /* Create new argv. */

    if (NULL == *argv) {
        *argv = (char**) malloc(2 * sizeof(char *));
        if (NULL == *argv) {
            return errno;
        }
        argc = 0;
        (*argv)[0] = NULL;
        (*argv)[1] = NULL;
    }

    /* Extend existing argv. */
    else {
        /* count how many entries currently exist */
        argc = geopm_argv_count(*argv);

        *argv = (char**) realloc(*argv, (argc + 2) * sizeof(char *));
        if (NULL == *argv) {
            return errno;
        }
    }

    /* Set the newest element to point to a copy of the arg string */

    (*argv)[argc] = strdup(arg);
    if (NULL == (*argv)[argc]) {
        return errno;
    }

    argc = argc + 1;
    (*argv)[argc] = NULL;

    return 0;
}

/*
 * creates a C string from an F77 string
 */
int geopm_fortran_string_f2c(char *fstr, int len, char **cstr)
{
    char *end;
    int i;

    /* Leading and trailing blanks are discarded. */

    end = fstr + len - 1;

    for (i = 0; (i < len) && (' ' == *fstr); ++i, ++fstr) {
        continue;
    }

    if (i >= len) {
        len = 0;
    }
    else {
        for (; (end > fstr) && (' ' == *end); --end) {
            continue;
        }

        len = end - fstr + 1;
    }

    /* Allocate space for the C string. */

    if (NULL == (*cstr = (char *) malloc(len + 1))) {
        return errno;
    }

    /* Copy F77 string into C string and NULL terminate it. */

    if (len > 0) {
        strncpy(*cstr, fstr, len);
    }
    (*cstr)[len] = '\0';

    return 0;
}


/*
 * Copy a C string into a Fortran string.  Note that when Fortran
 * copies strings, even if it operates on subsets of the strings, it
 * is expected to zero out the rest of the string with spaces.  Hence,
 * when calling this function, the "len" parameter should be the
 * cgeopmler-passed length of the entire string, even if you're copying
 * over less than the full string.  Specifically:
 *
 * http://www.ibiblio.org/pub/languages/fortran/ch2-13.html
 *
 * "Whole operations 'using' only 'part' of it, e.g. assignment of a
 * shorter string, or reading a shorter record, automatically pads the
 * rest of the string with blanks."
 */
int geopm_fortran_string_c2f(char *cstr, char *fstr, int len)
{
    int i;

    strncpy(fstr, cstr, len);
    for (i = strlen(cstr); i < len; ++i) {
        fstr[i] = ' ';
    }

    return 0;
}


/*
 * creates a C argument vector from an F77 array of strings
 * (terminated by a blank string)
 */
int geopm_fortran_argv_f2c(char *array, int string_len, int advance,
                           char ***argv)
{
    int err, argc = 0;
    char *cstr;

    /* Fortran lines up strings in memory, each delimited by \0.  So
       just convert them until we hit an extra \0. */

    *argv = NULL;
    while (1) {
        if (0 != (err = geopm_fortran_string_f2c(array, string_len, &cstr))) {
            geopm_argv_free(*argv);
            return err;
        }

        if ('\0' == *cstr) {
            break;
        }

        if (0 != (err = geopm_argv_append(&argc, argv, cstr))) {
            geopm_argv_free(*argv);
            free(cstr);
            return err;
        }

        free(cstr);
        array += advance;
    }

    free(cstr);
    return 0;
}


/*
 * Creates a set of C argv arrays from an F77 array of argv's.  The
 * returned arrays need to be freed by the caller.
 */
int geopm_fortran_multiple_argvs_f2c(int num_argv_arrays, char *array,
                                     int string_len, char ****argv)
{
    char ***argv_array;
    int i;
    char *current_array = array;
    int ret;

    argv_array = (char ***) malloc (num_argv_arrays * sizeof(char **));

    for (i = 0; i < num_argv_arrays; ++i) {
        ret = geopm_fortran_argv_f2c(current_array, string_len,
                                     string_len * num_argv_arrays,
                                     &argv_array[i]);
        if (0 != ret) {
            free(argv_array);
            return ret;
        }
        current_array += string_len;
    }
    *argv = argv_array;
    return 0;
}
