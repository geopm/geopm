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

! This file created from test/mpi/f77/coll/nonblockingf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer NUM_INTS
      parameter (NUM_INTS=2)
      integer maxSize
      parameter (maxSize=128)
      integer scounts(maxSize), sdispls(maxSize)
      integer rcounts(maxSize), rdispls(maxSize)
      integer types(maxSize)
      integer sbuf(maxSize), rbuf(maxSize)
      integer comm, size, rank, req
      integer ierr, errs
      integer ii, ans

      errs = 0

      call mtest_init(ierr)

      comm = MPI_COMM_WORLD
      call MPI_Comm_size(comm, size, ierr)
      call MPI_Comm_rank(comm, rank, ierr)
!      
      do ii = 1, size
         sbuf(2*ii-1) = ii
         sbuf(2*ii)   = ii
         sbuf(2*ii-1) = ii
         sbuf(2*ii)   = ii
         scounts(ii)  = NUM_INTS
         rcounts(ii)  = NUM_INTS
         sdispls(ii)  = (ii-1) * NUM_INTS
         rdispls(ii)  = (ii-1) * NUM_INTS
         types(ii)    = MPI_INTEGER
      enddo

      call MPI_Ibarrier(comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ibcast(sbuf, NUM_INTS, MPI_INTEGER, 0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Igather(sbuf, NUM_INTS, MPI_INTEGER, &
      &                  rbuf, NUM_INTS, MPI_INTEGER, &
      &                  0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Igatherv(sbuf, NUM_INTS, MPI_INTEGER, &
      &                   rbuf, rcounts, rdispls, MPI_INTEGER, &
      &                   0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoall(sbuf, NUM_INTS, MPI_INTEGER, &
      &                    rbuf, NUM_INTS, MPI_INTEGER, &
      &                    comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoallv(sbuf, scounts, sdispls, MPI_INTEGER, &
      &                     rbuf, rcounts, rdispls, MPI_INTEGER, &
      &                     comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ialltoallw(sbuf, scounts, sdispls, types, &
      &                     rbuf, rcounts, rdispls, types, &
      &                     comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce(sbuf, rbuf, NUM_INTS, MPI_INTEGER, &
      &                  MPI_SUM, 0, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iallreduce(sbuf, rbuf, NUM_INTS, MPI_INTEGER, &
      &                     MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce_scatter(sbuf, rbuf, rcounts, MPI_INTEGER, &
      &                          MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Ireduce_scatter_block(sbuf, rbuf, NUM_INTS, MPI_INTEGER, &
      &                                MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iscan(sbuf, rbuf, NUM_INTS, MPI_INTEGER, &
      &                MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call MPI_Iexscan(sbuf, rbuf, NUM_INTS, MPI_INTEGER, &
      &                  MPI_SUM, comm, req, ierr)
      call MPI_Wait(req, MPI_STATUS_IGNORE, ierr)

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
