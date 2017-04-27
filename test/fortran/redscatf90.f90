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

! This file created from test/mpi/f77/coll/redscatf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      subroutine uop( cin, cout, count, datatype )
      use mpi
      integer cin(*), cout(*)
      integer count, datatype
      integer i
      
      if (datatype .ne. MPI_INTEGER) then
         write(6,*) 'Invalid datatype ',datatype,' passed to user_op()'
         return
      endif

      do i=1, count
         cout(i) = cin(i) + cout(i)
      enddo
      end
!
! Test of reduce scatter.
!
! Each processor contributes its rank + the index to the reduction, 
! then receives the ith sum
!
! Can be called with any number of processors.
!

      program main
      use mpi
      integer errs, ierr, toterr
      integer maxsize
      parameter (maxsize=1024)
      integer sendbuf(maxsize), recvbuf, recvcounts(maxsize)
      integer size, rank, i, sumval
      integer comm, sumop
      external uop

      errs = 0

      call mtest_init( ierr )

      comm = MPI_COMM_WORLD

      call mpi_comm_size( comm, size, ierr )
      call mpi_comm_rank( comm, rank, ierr )

      if (size .gt. maxsize) then
      endif
      do i=1, size
         sendbuf(i) = rank + i - 1
         recvcounts(i) = 1
      enddo

      call mpi_reduce_scatter( sendbuf, recvbuf, recvcounts,  &
      &     MPI_INTEGER, MPI_SUM, comm, ierr )

      sumval = size * rank + ((size - 1) * size)/2
! recvbuf should be size * (rank + i) 
      if (recvbuf .ne. sumval) then
         errs = errs + 1
         print *, "Did not get expected value for reduce scatter"
         print *, rank, " Got ", recvbuf, " expected ", sumval
      endif

      call mpi_op_create( uop, .true., sumop, ierr )
      call mpi_reduce_scatter( sendbuf, recvbuf, recvcounts,  &
      &     MPI_INTEGER, sumop, comm, ierr )

      sumval = size * rank + ((size - 1) * size)/2
! recvbuf should be size * (rank + i) 
      if (recvbuf .ne. sumval) then
         errs = errs + 1
         print *, "sumop: Did not get expected value for reduce scatter"
         print *, rank, " Got ", recvbuf, " expected ", sumval
      endif
      call mpi_op_free( sumop, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
