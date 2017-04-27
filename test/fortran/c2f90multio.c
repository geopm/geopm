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

/* This file created from test/mpi/f77/io/c2fmultio.c with f77tof90 */
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
  Check that MPI_File_c2f, applied to the same object several times,
  yields the same handle.  We do this because when MPI handles in 
  C are a different length than those in Fortran, care needs to 
  be exercised to ensure that the mapping from one to another is unique.
  (Test added to test a potential problem in ROMIO for handling MPI_File
  on 64-bit systems)
*/
#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    MPI_Fint handleA, handleB;
    int      rc;
    int      errs = 0;
    int      rank;
    MPI_File cFile;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    /* File */
    rc = MPI_File_open( MPI_COMM_WORLD, (char*)"temp", 
		   MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE | MPI_MODE_CREATE, 
		   MPI_INFO_NULL, &cFile );
    if (rc) {
	errs++;
	printf( "Unable to open file \"temp\"\n" );
    }
    else {
	handleA = MPI_File_c2f( cFile );
	handleB = MPI_File_c2f( cFile );
	if (handleA != handleB) {
	    errs++;
	    printf( "MPI_File_c2f does not give the same handle twice on the same MPI_File\n" );
	}
    }
    MPI_File_close( &cFile );

    if (rank == 0) {
	if (errs) {
	    fprintf(stderr, "Found %d errors\n", errs);
	}
	else {
	    printf(" No Errors\n");
	}
    }
    
    MPI_Finalize();
    
    return 0;
}
