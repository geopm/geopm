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

! This file created from test/mpi/f77/coll/inplacef.f with f77tof90
! -*- Mode: Fortran; -*-
!
! (C) 2005 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! This is a simple test that Fortran support the MPI_IN_PLACE value
!
       program main
       use mpi
       integer ierr, errs
       integer comm, root
       integer rank, size
       integer i
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer rbuf(MAX_SIZE), rdispls(MAX_SIZE), rcount(MAX_SIZE), &
      &      sbuf(MAX_SIZE)

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )

       root = 0
! Gather with inplace
       do i=1,size
          rbuf(i) = - i
       enddo
       rbuf(1+root) = root
       if (rank .eq. root) then
          call mpi_gather( MPI_IN_PLACE, 1, MPI_INTEGER, rbuf, 1, &
      &         MPI_INTEGER, root, comm, ierr )
          do i=1,size
             if (rbuf(i) .ne. i-1) then
                errs = errs + 1
                print *, '[',rank,'] rbuf(', i, ') = ', rbuf(i),  &
      &                   ' in gather'
             endif
          enddo
       else
          call mpi_gather( rank, 1, MPI_INTEGER, rbuf, 1, MPI_INTEGER, &
      &         root, comm, ierr )
       endif

! Gatherv with inplace
       do i=1,size
          rbuf(i) = - i
          rcount(i) = 1
          rdispls(i) = i-1
       enddo
       rbuf(1+root) = root
       if (rank .eq. root) then
          call mpi_gatherv( MPI_IN_PLACE, 1, MPI_INTEGER, rbuf, rcount, &
      &         rdispls, MPI_INTEGER, root, comm, ierr )
          do i=1,size
             if (rbuf(i) .ne. i-1) then
                errs = errs + 1
                print *, '[', rank, '] rbuf(', i, ') = ', rbuf(i),  &
      &                ' in gatherv'
             endif
          enddo
       else
          call mpi_gatherv( rank, 1, MPI_INTEGER, rbuf, rcount, rdispls, &
      &         MPI_INTEGER, root, comm, ierr )
       endif

! Scatter with inplace
       do i=1,size
          sbuf(i) = i
       enddo
       rbuf(1) = -1
       if (rank .eq. root) then
          call mpi_scatter( sbuf, 1, MPI_INTEGER, MPI_IN_PLACE, 1, &
      &         MPI_INTEGER, root, comm, ierr )
       else
          call mpi_scatter( sbuf, 1, MPI_INTEGER, rbuf, 1, &
      &         MPI_INTEGER, root, comm, ierr )
          if (rbuf(1) .ne. rank+1) then
             errs = errs + 1
             print *, '[', rank, '] rbuf  = ', rbuf(1), &
      &            ' in scatter'
          endif
       endif

       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
