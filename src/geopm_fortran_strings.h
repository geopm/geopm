/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
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
 *- Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer listed
 *  in this license in the documentation and/or other materials
 *  provided with the distribution.
 *
 *- Neither the name of the copyright holders nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
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

#ifndef GEOPM_FORTRAN_STRINGS_H
#define GEOPM_FORTRAN_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Free a NULL-terminated argv array.
 *
 * @param argv Argv array to free.
 *
 * This function frees an argv array and all of the strings that it
 * contains.  Since the argv parameter is passed by value, it is not
 * set to NULL in the caller's scope upon return.
 *
 * It is safe to invoke this function with a NULL pointer.  It is
 * not safe to invoke this function with a non-NULL-terminated argv
 * array.
 */
void geopm_argv_free(char **argv);

/**
 * Append a string (by value) to an new or existing NULL-terminated
 * argv array.
 *
 * @param argc Pointer to the length of the argv array.  Must not be
 * NULL.
 * @param argv Pointer to an argv array.
 * @param str Pointer to the string to append.
 *
 * @retval 0 On success
 * @retval errno On failure
 *
 * This function adds a string to an argv array of strings by value;
 * it is permissable to pass a string on the stack as the str
 * argument to this function.
 *
 * To add the first entry to an argv array, call this function with
 * (*argv == NULL).  This function will allocate an array of length
 * 2; the first entry will point to a copy of the string passed in
 * arg, the second entry will be set to NULL.
 *
 * If (*argv != NULL), it will be realloc'ed to be 1 (char*) larger,
 * and the next-to-last entry will point to a copy of the string
 * passed in arg.  The last entry will be set to NULL.
 *
 * Just to reinforce what was stated above: the string is copied by
 * value into the argv array; there is no need to keep the original
 * string (i.e., the arg parameter) after invoking this function.
 */
int geopm_argv_append(int *argc, char ***argv, const char *arg);

/**
 * Append to an argv-style array, but ignore the size of the array.
 *
 * @param argv Pointer to an argv array.
 * @param str Pointer to the string to append.
 *
 * @retval 0 On success
 * @retval errno On failure
 *
 * This function is identical to the geopm_argv_append() function
 * except that it does not take a pointer to an argc (integer
 * representing the size of the array).  This is handy for
 * argv-style arrays that do not have integers that are actively
 * maintaing their sizes.
 */
int geopm_argv_append_nosize(char ***argv, const char *arg);

/**
 * Convert a fortran string to a C string.
 *
 * @param fstr Fortran string
 * @param len Fortran string length
 * @param cstr Pointer to C string that will be created and returned
 *
 * @retval 0 upon success
 * @retval errno upon error
 *
 * This function is intended to be used in the MPI F77 bindings to
 * convert fortran strings to C strings before invoking a back-end
 * MPI C binding function.  It will create a new C string and
 * assign it to the cstr to return.  The caller is responsible for
 * eventually freeing the C string.
 */
int geopm_fortran_string_f2c(char *fstr, int len, char **cstr);

/**
 * Convert a C string to a fortran string.
 *
 * @param cstr C string
 * @param fstr Fortran string (must already exist and be allocated)
 * @param len Fortran string length
 *
 * @retval 0 upon success
 * @retval errno upon error
 *
 * This function is intended to be used in the MPI F77 bindings to
 * convert C strings to fortran strings.  It is assumed that the
 * fortran string is already allocated and has a length of len.
 */
int geopm_fortran_string_c2f(char *cstr, char *fstr, int len);

/**
 * Convert an array of Fortran strings to an argv-style array of C
 * strings.
 *
 * @param farray Array of fortran strings
 * @param string_len Length of each fortran string in the array
 * @param advance Number of bytes to advance to get to the next string
 * @param cargv Returned argv-style array of C strings
 *
 * @retval 0 upon success
 * @retval errno upon error
 *
 * This function is intented to be used in the MPI F77 bindings to
 * convert arrays of fortran strings to argv-style arrays of C
 * strings.  The argv array will be allocated and returned; it is
 * the caller's responsibility to invoke geopm_argv_free() to free
 * it later (or equivalent).
 *
 * For 1D Fortran string arrays, advance will == string_len.
 *
 * However, when this function is used (indirectly) for
 * MPI_COMM_SPAWN_MULTIPLE, a 2D array of Fortran strings is
 * converted to individual C-style argv vectors.  In this case,
 * Fortran will intertwine the strings of the different argv
 * vectors in memory; the displacement between the beginning of 2
 * strings in a single argv vector is (string_len *
 * number_of_argv_arrays).  Hence, the advance parameter is used
 * to specify this displacement.
 */
int geopm_fortran_argv_f2c(char *farray, int string_len,
                           int advancex, char ***cargv);

/**
 * Convert an array of argvs to a C style array of argvs
 * @param count Dimension of the array of argvs
 * @param array Array of fortran argv
 * @param len Length of Fortran array
 * @param argv Returned C arrray of argvs
 *
 * This function is intented to be used in the MPI F77 bindings to
 * convert arrays of fortran strings to argv-style arrays of C
 * strings.  The argv array will be allocated and returned; it is
 * the caller's responsibility to invoke geopm_argv_free() to free
 * each content of argv array and call free to deallocate the argv
 * array itself
 */
int geopm_fortran_multiple_argvs_f2c(int count, char *array, int len,
                                     char ****argv);

#ifdef __cplusplus
}
#endif
#endif /* GEOPM_FORTRAN_STRINGS_H */
