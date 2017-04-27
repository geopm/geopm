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

! This file created from test/mpi/f77/coll/vw_inplacef.f with f77tof90
! -*- Mode: Fortran; -*- 
!
! (C) 2012 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! A simple test for Fortran support of the MPI_IN_PLACE value in Alltoall[vw].
!
       program main
       use mpi
       integer SIZEOFINT
       integer MAX_SIZE
       parameter (MAX_SIZE=1024)
       integer rbuf(MAX_SIZE)
       integer rdispls(MAX_SIZE), rcounts(MAX_SIZE), rtypes(MAX_SIZE)
       integer ierr, errs
       integer comm, root
       integer rank, size
       integer iexpected, igot
       integer i, j

       errs = 0
       call mtest_init( ierr )

       comm = MPI_COMM_WORLD
       call mpi_comm_rank( comm, rank, ierr )
       call mpi_comm_size( comm, size, ierr )
       call mpi_type_size( MPI_INTEGER, SIZEOFINT, ierr )

       if (size .gt. MAX_SIZE) then
          print *, ' At most ', MAX_SIZE, ' processes allowed'
          call mpi_abort( MPI_COMM_WORLD, 1, ierr )
       endif
!
       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i=1,size
          rbuf(i) = (i-1) * size + rank
       enddo
       call mpi_alltoall( MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, &
      &      rbuf, 1, MPI_INTEGER, comm, ierr )
       do i=1,size
          if (rbuf(i) .ne. (rank*size + i - 1)) then
             errs = errs + 1
             print *, '[', rank, '] rbuf(', i, ') = ', rbuf(i), &
      &             ', should be', rank * size + i - 1
          endif
       enddo

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
       do i=1,size
           rcounts(i) = (i-1) + rank
           rdispls(i) = (i-1) * (2*size)
           do j=0,rcounts(i)-1
               rbuf(rdispls(i)+j+1) = 100 * rank + 10 * (i-1) + j
           enddo
       enddo
       call mpi_alltoallv( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL, &
      &                     rbuf, rcounts, rdispls, MPI_INTEGER, &
      &                     comm, ierr )
       do i=1,size
           do j=0,rcounts(i)-1
               iexpected = 100 * (i-1) + 10 * rank + j
               igot      = rbuf(rdispls(i)+j+1)
               if ( igot .ne. iexpected ) then
                   errs = errs + 1
                   print *, '[', rank, '] ALLTOALLV got ', igot, &
      &                   ',but expected ', iexpected, &
      &                   ' for block=', i-1, ' element=', j
               endif
           enddo
       enddo

       do i=1,MAX_SIZE
           rbuf(i) = -1
       enddo
!          Alltoallw's displs[] are in bytes not in type extents.
       do i=1,size
           rcounts(i) = (i-1) + rank
           rdispls(i) = (i-1) * (2*size) * SIZEOFINT
           rtypes(i)  = MPI_INTEGER
           do j=0,rcounts(i)-1
               rbuf(rdispls(i)/SIZEOFINT+j+1) = 100 * rank &
      &                                        + 10 * (i-1) + j
           enddo
       enddo
       call mpi_alltoallw( MPI_IN_PLACE, 0, 0, MPI_DATATYPE_NULL, &
      &                     rbuf, rcounts, rdispls, rtypes, &
      &                     comm, ierr )
       do i=1,size
           do j=0,rcounts(i)-1
               iexpected = 100 * (i-1) + 10 * rank + j
               igot      = rbuf(rdispls(i)/SIZEOFINT+j+1)
               if ( igot .ne. iexpected ) then
                   errs = errs + 1
                   print *, '[', rank, '] ALLTOALLW got ', igot, &
      &                   ',but expected ', iexpected, &
      &                   ' for block=', i-1, ' element=', j
               endif
           enddo
       enddo

       call mtest_finalize( errs )
       call mpi_finalize( ierr )

       end
