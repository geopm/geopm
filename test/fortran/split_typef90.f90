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

! This file created from test/mpi/f77/coll/split_typef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer ierr, errs
      integer i, ans, size, rank, color, comm, newcomm
      integer maxSize, displ
      parameter (maxSize=128)
      integer scounts(maxSize), sdispls(maxSize), stypes(maxSize)
      integer rcounts(maxSize), rdispls(maxSize), rtypes(maxSize)
      integer sbuf(maxSize), rbuf(maxSize)

      errs = 0

      call mtest_init( ierr )

      call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )

      call mpi_comm_split_type( comm, MPI_COMM_TYPE_SHARED, rank, &
      &     MPI_INFO_NULL, newcomm, ierr )
      call mpi_comm_rank( newcomm, rank, ierr )
      call mpi_comm_size( newcomm, size, ierr )

      do i=1, size
         scounts(i) = 1
         sdispls(i) = (i-1)
         stypes(i)  = MPI_INTEGER
         sbuf(i) = rank * size + i
         rcounts(i) = 1
         rdispls(i) = (i-1)
         rtypes(i)  = MPI_INTEGER
         rbuf(i) = -1
      enddo
      call mpi_alltoallv( sbuf, scounts, sdispls, stypes, &
      &     rbuf, rcounts, rdispls, rtypes, newcomm, ierr )

      call mpi_comm_free( newcomm, ierr )
      call mpi_comm_free( comm, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
