!
! MVAPICH2
!
! Copyright 2003-2016 The Ohio State University.
! Portions Copyright 1999-2002 The Regents of the University of
! California, through Lawrence Berkeley National Laboratory (subject to
! receipt of any required approvals from U.S. Dept. of Energy).
! Portions copyright 1993 University of Chicago.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are
! met:
!
! (1) Redistributions of source code must retain the above copyright
! notice, this list of conditions and the following disclaimer.
!
! (2) Redistributions in binary form must reproduce the above copyright
! notice, this list of conditions and the following disclaimer in the
! documentation and/or other materials provided with the distribution.
!
! (3) Neither the name of The Ohio State University, the University of
! California, Lawrence Berkeley National Laboratory, The University of
! Chicago, Argonne National Laboratory, U.S. Dept. of Energy nor the
! names of their contributors may be used to endorse or promote products
! derived from this software without specific prior written permission.
!
! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
! "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
! LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
! A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
! OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
! SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
! LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
! DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
! THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!

! This file created from test/mpi/f77/ext/allocmemf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi
!
! This program makes use of a common (but not universal; g77 doesn't
! have it) extension: the "Cray" pointer.  This allows MPI_Alloc_mem
! to allocate memory and return it to Fortran, where it can be used.
! As this is not standard Fortran, this test is not run by default.
! To run it, build (with a suitable compiler) and run with
!   mpiexec -n 1 ./allocmemf
!
        real a
        pointer (p,a(100,100))
        integer (kind=MPI_ADDRESS_KIND) asize

        integer ierr, sizeofreal, errs
        integer i,j
!
        errs = 0
        call mtest_init(ierr)
        call mpi_type_size( MPI_REAL, sizeofreal, ierr )
! Make sure we pass in an integer of the correct type
        asize = sizeofreal * 100 * 100
        call mpi_alloc_mem( asize,MPI_INFO_NULL,p,ierr )

        do i=1,100
            do j=1,100
                a(i,j) = -1
            enddo
        enddo
        a(3,5) = 10.0

        call mpi_free_mem( a, ierr )
        call mtest_finalize(errs)
        call mpi_finalize(ierr)

        end
