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

! This file created from test/mpi/f77/coll/red_scat_blockf.f with f77tof90
! -*- Mode: Fortran; -*-
!
! (C) 2012 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! A simple test for Fortran support of Reduce_scatter_block
! with or withoutMPI_IN_PLACE.
!
       program main
       use mpi
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer sbuf(MAX_SIZE), rbuf(MAX_SIZE)
       integer comm, rank, size
       integer sumval, ierr, errs, i

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )

       do i = 1, size
           sbuf(i) = rank + (i-1)
       enddo

       call MPI_Reduce_scatter_block(sbuf, rbuf, 1, MPI_INTEGER, &
      &                               MPI_SUM, comm, ierr)

       sumval = size * rank + ((size-1) * size)/2
       if ( rbuf(1) .ne. sumval ) then
           errs = errs + 1
           print *, 'Reduce_scatter_block does not get expected value.'
           print *, '[', rank, ']', 'Got ', rbuf(1), ' but expected ', &
      &              sumval, '.'
       endif

! Try MPI_IN_PLACE
       do i = 1, size
           rbuf(i) = rank + (i-1)
       enddo
       call MPI_Reduce_scatter_block(MPI_IN_PLACE, rbuf, 1, MPI_INTEGER, &
      &                               MPI_SUM, comm, ierr)
       if ( rbuf(1) .ne. sumval ) then
           errs = errs + 1
           print *, 'Reduce_scatter_block does not get expected value.'
           print *, '[', rank, ']', 'Got ', rbuf(1), ' but expected ', &
      &              sumval, '.'
       endif

       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
