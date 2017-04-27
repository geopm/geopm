/*
 * MVAPICH2
 * 
 * Copyright 2003-2016 The Ohio State University.
 * Portions Copyright 1999-2002 The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject to
 * receipt of any required approvals from U.S. Dept. of Energy).
 * Portions copyright 1993 University of Chicago.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * (1) Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * (2) Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * (3) Neither the name of The Ohio State University, the University of
 * California, Lawrence Berkeley National Laboratory, The University of
 * Chicago, Argonne National Laboratory, U.S. Dept. of Energy nor the
 * names of their contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
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

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style: allow:fprintf:10 sig:0 */
#include <stdio.h>
#include "mpi.h"
#include "mpitestconf.h"
#include <string.h>

/* 
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern 
*/

#ifdef F77_NAME_UPPER
#define chkcomm2inc_ CHKCOMM2INC
#define chkckeyvals_ CHKCKEYVALS

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define chkcomm2inc_ chkcomm2inc
#define chkckeyvals_ chkckeyvals

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else 
#error 'Unrecognized Fortran name mapping'
#endif


int chkcomm2inc_ (int *keyval, const int *expected, int *ierr);
int chkcomm2inc_ (int *keyval, const int *expected, int *ierr)
{
    int      flag;
    MPI_Aint *val;

    /* See Example 16.19 in MPI 2.2, part B.  The use of MPI_Aint *val
       and the address of val in the get_attr call is correct, as is
       the use of *val to access the value. */
    MPI_Comm_get_attr( MPI_COMM_WORLD, *keyval, &val, &flag );
    if (!flag) {
	*ierr = 1;
    }
    else {
	if (*val != *expected) {
	    /* In some cases, using printf from a c routine linked 
	       with a Fortran routine can cause linking difficulties.
	       To avoid problems in running the tests, this print
	       is commented out */
	    /* printf( "Val = %x, expected = %d\n", val, *expected ); */
	    *ierr = *ierr + 1;
	}
    }
    return 0;
}

/* Attribute delete and copy functions for each type */
int myCommCopyfn( MPI_Comm comm, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag );
int myCommCopyfn( MPI_Comm comm, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag )
{
    *(void **)attr_val_out = (char *)attr_val_in + 2;
    *flag = 1;
    return MPI_SUCCESS;
}

int myCommDelfn( MPI_Comm comm, int keyval, void *attr_val, void *extra_state );
int myCommDelfn( MPI_Comm comm, int keyval, void *attr_val, void *extra_state )
{
    return MPI_SUCCESS;
}

int myTypeCopyfn( MPI_Datatype dtype, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag );
int myTypeCopyfn( MPI_Datatype dtype, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag )
{
    *(void **)attr_val_out = (char *)attr_val_in + 2;
    *flag = 1;
    return MPI_SUCCESS;
}

int myTypeDelfn( MPI_Datatype dtype, int keyval, void *attr_val, void *extra_state );
int myTypeDelfn( MPI_Datatype dtype, int keyval, void *attr_val, void *extra_state )
{
    return MPI_SUCCESS;
}

int myWinCopyfn( MPI_Win win, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag );
int myWinCopyfn( MPI_Win win, int keyval, void *extra_state, 
		  void *attr_val_in, void *attr_val_out, int *flag )
{
    *(void **)attr_val_out = (char *)attr_val_in + 2;
    *flag = 1;
    return MPI_SUCCESS;
}

int myWinDelfn( MPI_Win win, int keyval, void *attr_val, void *extra_state );
int myWinDelfn( MPI_Win win, int keyval, void *attr_val, void *extra_state )
{
    return MPI_SUCCESS;
}

int chkckeyvals_( int *comm_keyval, int *type_keyval, int *win_keyval );
int chkckeyvals_( int *comm_keyval, int *type_keyval, int *win_keyval )
{
    MPI_Comm_create_keyval( myCommCopyfn, myCommDelfn, comm_keyval, 0 );
    MPI_Type_create_keyval( myTypeCopyfn, myTypeDelfn, type_keyval, 0 );
    MPI_Win_create_keyval( myWinCopyfn, myWinDelfn, win_keyval, 0 );
    return 0;
}
