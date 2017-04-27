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

! -*- Mode: Fortran; -*-
!
!  (C) 2007 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program tests that the MPI_SIZEOF routine is implemented for the
! predefined scalar Fortran types.  It confirms that the size of these
! types matches the size of the corresponding MPI datatypes.
!
      program main
      use mpi
      integer ierr, errs
      integer rank, size, mpisize
      logical verbose
      real    r1,r1v(2)
      double precision d1,d1v(3)
      complex c1,c1v(4)
      integer i1,i1v(5)
      character ch1,ch1v(6)
      logical l1,l1v(7)

      verbose = .false.
      errs = 0
      call mtest_init ( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )

! Test of scalar types
      call mpi_sizeof( r1, size, ierr )
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( d1, size, ierr )
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( i1, size, ierr )
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( c1, size, ierr )
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( ch1, size, ierr )
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( l1, size, ierr )
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but MPI_SIZEOF gives ", size
      endif
!
! Test of vector types (1-dimensional)
      call mpi_sizeof( r1v, size, ierr )
      call mpi_type_size( MPI_REAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_REAL = ", mpisize,                         &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( d1v, size, ierr )
      call mpi_type_size( MPI_DOUBLE_PRECISION, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_DOUBLE_PRECISION = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( i1v, size, ierr )
      call mpi_type_size( MPI_INTEGER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_INTEGER = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( c1v, size, ierr )
      call mpi_type_size( MPI_COMPLEX, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_COMPLEX = ", mpisize,                      &
     &            " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( ch1v, size, ierr )
      call mpi_type_size( MPI_CHARACTER, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_CHARACTER = ", mpisize, &
              " but MPI_SIZEOF gives ", size
      endif

      call mpi_sizeof( l1v, size, ierr )
      call mpi_type_size( MPI_LOGICAL, mpisize, ierr )
      if (size .ne. mpisize) then
         errs = errs + 1
         print *, "Size of MPI_LOGICAL = ", mpisize,                        &
     &        " but MPI_SIZEOF gives ", size
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
