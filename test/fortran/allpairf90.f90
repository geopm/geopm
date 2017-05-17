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

! This file created from test/mpi/f77/pt2pt/allpairf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program is based on the allpair.f test from the MPICH-1 test
! (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
! fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program allpair
      use mpi
      integer ierr, errs, comm
      logical mtestGetIntraComm
      logical verbose
      common /flags/ verbose

      errs = 0
      verbose = .false.
!      verbose = .true.
      call MTest_Init( ierr )

      do while ( mtestGetIntraComm( comm, 2, .false. ) )
         call test_pair_send( comm, errs )
         call test_pair_ssend( comm, errs )
         call test_pair_rsend( comm, errs )
         call test_pair_isend( comm, errs )
         call test_pair_irsend( comm, errs )
         call test_pair_issend( comm, errs )
         call test_pair_psend( comm, errs )
         call test_pair_prsend( comm, errs )
         call test_pair_pssend( comm, errs )
         call test_pair_sendrecv( comm, errs )
         call test_pair_sendrecvrepl( comm, errs )
         call mtestFreeComm( comm )
      enddo
!
      call MTest_Finalize( errs )
      call MPI_Finalize(ierr)
!
      end
!
      subroutine test_pair_send( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Send and recv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 1123
      count = TEST_SIZE / 5
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Send(send_buf, count, MPI_REAL, next, tag, &
      &        comm, ierr)
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, status, ierr)
!
         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'send and recv', errs )
      else if (prev .eq. 0)  then
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'send and recv', errs )
!
         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, comm, ierr)
      end if
!
      end
!
      subroutine test_pair_rsend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(1)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Rsend and recv'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 1456
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Recv( MPI_BOTTOM, 0, MPI_INTEGER, next, tag,  &
      &                  comm, status, ierr )
!
         call MPI_Rsend(send_buf, count, MPI_REAL, next, tag, &
      &                  comm, ierr)
!
         call MPI_Probe(MPI_ANY_SOURCE, tag, comm, status, ierr)
!
         if (status(MPI_SOURCE) .ne. next) then
            print *, 'Rsend: Incorrect source, expected', next, &
      &               ', got', status(MPI_SOURCE)
            errs = errs + 1
         end if
!
         if (status(MPI_TAG) .ne. tag) then
            print *, 'Rsend: Incorrect tag, expected', tag, &
      &               ', got', status(MPI_TAG)
            errs = errs + 1
         end if
!
         call MPI_Get_count(status, MPI_REAL, i, ierr)
!
         if (i .ne. count) then
            print *, 'Rsend: Incorrect count, expected', count, &
      &               ', got', i
            errs = errs + 1
         end if
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,  &
      &                 status, ierr)
!
         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'rsend and recv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 requests(1), ierr)
         call MPI_Send( MPI_BOTTOM, 0, MPI_INTEGER, prev, tag,  &
      &                  comm, ierr )
         call MPI_Wait( requests(1), status, ierr )
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'rsend and recv', errs )
!
         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, &
      &                  comm, ierr)
      end if
!
      end
!
      subroutine test_pair_ssend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Ssend and recv'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 1789
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Iprobe(MPI_ANY_SOURCE, tag, &
      &                   comm, flag, status, ierr)
!
         if (flag) then
            print *, 'Ssend: Iprobe succeeded! source',  &
      &               status(MPI_SOURCE), &
      &               ', tag', status(MPI_TAG)
            errs = errs + 1
         end if
!
         call MPI_Ssend(send_buf, count, MPI_REAL, next, tag, &
      &                  comm, ierr)
!
         do while (.not. flag)
            call MPI_Iprobe(MPI_ANY_SOURCE, tag, &
      &                      comm, flag, status, ierr)
         end do
!
         if (status(MPI_SOURCE) .ne. next) then
            print *, 'Ssend: Incorrect source, expected', next, &
      &               ', got', status(MPI_SOURCE)
            errs = errs + 1
         end if
!
         if (status(MPI_TAG) .ne. tag) then
            print *, 'Ssend: Incorrect tag, expected', tag, &
      &               ', got', status(MPI_TAG)
            errs = errs + 1
         end if
!
         call MPI_Get_count(status, MPI_REAL, i, ierr)
!
         if (i .ne. count) then
            print *, 'Ssend: Incorrect count, expected', count, &
      &               ', got', i
            errs = errs + 1
         end if
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)
!
         call msg_check( recv_buf, next, tag, count, status, &
      &        TEST_SIZE, 'ssend and recv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)
!
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'ssend and recv', errs )
!
         call MPI_Ssend(recv_buf, count, MPI_REAL, prev, tag, &
      &                  comm, ierr)
      end if
!
      end
!
      subroutine test_pair_isend( comm, errs )
      use mpi
      use iso_fortran_env
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' isend and irecv'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 2123
      count = TEST_SIZE / 5
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call mpi_irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call mpi_isend(send_buf, count, MPI_REAL, next, tag, &
      &                  comm, requests(2), ierr)
!
         call MPI_Waitall(2, requests, statuses, ierr)
         status = statuses(1,1)
!
         call rq_check( requests, 2, 'isend and irecv' )
!
         call msg_check( recv_buf, next, tag, count, statuses(1,1), &
      &        TEST_SIZE, 'isend and irecv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)
!
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'isend and irecv', errs )
!
         call MPI_Isend(recv_buf, count, MPI_REAL, prev, tag, &
      &                  comm, requests(1), ierr)
!
         call MPI_Wait(requests(1), status, ierr)
!
         call rq_check( requests(1), 1, 'isend and irecv' )
!
      end if
!
      end
!
      subroutine test_pair_irsend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer TEST_SIZE
      integer dupcom
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Irsend and irecv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      call mpi_comm_dup( comm, dupcom, ierr )
!
      tag = 2456
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, next, 0,  &
      &                      MPI_BOTTOM, 0, MPI_INTEGER, next, 0,  &
      &                      dupcom, status, ierr )
!
         call MPI_Irsend(send_buf, count, MPI_REAL, next, tag, &
      &                   comm, requests(2), ierr)
!
         index = -1
         do while (index .ne. 1)
            call MPI_Waitany(2, requests, index, statuses, ierr)
         end do
!
         call rq_check( requests(1), 1, 'irsend and irecv' )
!
         call msg_check( recv_buf, next, tag, count, statuses, &
      &           TEST_SIZE, 'irsend and irecv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, prev, 0,  &
      &                      MPI_BOTTOM, 0, MPI_INTEGER, prev, 0,  &
      &                      dupcom, status, ierr )
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Test(requests(1), flag, status, ierr)
         end do
!
         call rq_check( requests, 1, 'irsend and irecv (test)' )
!
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'irsend and irecv', errs )
!
         call MPI_Irsend(recv_buf, count, MPI_REAL, prev, tag, &
      &                   comm, requests(1), ierr)
!
         call MPI_Waitall(1, requests, statuses, ierr)
!
         call rq_check( requests, 1, 'irsend and irecv' )
!
      end if
!
      call mpi_comm_free( dupcom, ierr )
!
      end
!
      subroutine test_pair_issend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' issend and irecv (testall)'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 2789
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Issend(send_buf, count, MPI_REAL, next, tag, &
      &                   comm, requests(2), ierr)
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testall(2, requests, flag, statuses, ierr)
         end do
!
         call rq_check( requests, 2, 'issend and irecv (testall)' )
!
         call msg_check( recv_buf, next, tag, count, statuses(1,1), &
      &           TEST_SIZE, 'issend and recv (testall)', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'issend and recv', errs )

         call MPI_Issend(recv_buf, count, MPI_REAL, prev, tag, &
      &                   comm, requests(1), ierr)
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testany(1, requests(1), index, flag, &
      &                       statuses(1,1), ierr)
         end do
!
         call rq_check( requests, 1, 'issend and recv (testany)' )
!
      end if
!
      end
!
      subroutine test_pair_psend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      integer statuses(MPI_STATUS_SIZE,2), requests(2)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Persistent send and recv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 3123
      count = TEST_SIZE / 5
!
      call clear_test_data(recv_buf,TEST_SIZE)
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL, &
      &                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                   requests(2), ierr)
!
      if (rank .eq. 0) then
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Send_init(send_buf, count, MPI_REAL, next, tag, &
      &                      comm, requests(1), ierr)
!
         call MPI_Startall(2, requests, ierr)
         call MPI_Waitall(2, requests, statuses, ierr)
!
         call msg_check( recv_buf, next, tag, count, statuses(1,2), &
      &        TEST_SIZE, 'persistent send/recv', errs )
!
         call MPI_Request_free(requests(1), ierr)
!
      else if (prev .eq. 0) then
!
         call MPI_Send_init(send_buf, count, MPI_REAL, prev, tag, &
      &                      comm, requests(1), ierr)
         call MPI_Start(requests(2), ierr)
         call MPI_Wait(requests(2), status, ierr)
!
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'persistent send/recv', errs )
!
         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
!
         call MPI_Start(requests(1), ierr)
         call MPI_Wait(requests(1), status, ierr)
!
         call MPI_Request_free(requests(1), ierr)
      end if
!
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(2), ierr)
!
      end
!
      subroutine test_pair_prsend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer outcount, indices(2)
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer statuses(MPI_STATUS_SIZE,2), requests(2)
      integer status(MPI_STATUS_SIZE)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Persistent Rsend and recv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 3456
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL, &
      &                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                   requests(2), ierr)
!
      if (rank .eq. 0) then
!
         call MPI_Rsend_init(send_buf, count, MPI_REAL, next, tag, &
      &                       comm, requests(1), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Recv( MPI_BOTTOM, 0, MPI_INTEGER, next, tag,  &
      &                  comm, status, ierr )
!
         call MPI_Startall(2, requests, ierr)
!
         index = -1
!
         do while (index .ne. 2)
            call MPI_Waitsome(2, requests, outcount, &
      &                        indices, statuses, ierr)
            do i = 1,outcount
               if (indices(i) .eq. 2) then
                  call msg_check( recv_buf, next, tag, count, &
      &                 statuses(1,i), TEST_SIZE, 'waitsome', errs )
                  index = 2
               end if
            end do
         end do
!
         call MPI_Request_free(requests(1), ierr)
      else if (prev .eq. 0) then
!
         call MPI_Rsend_init(send_buf, count, MPI_REAL, prev, tag, &
      &                       comm, requests(1), ierr)
!
         call MPI_Start(requests(2), ierr)
!
         call MPI_Send( MPI_BOTTOM, 0, MPI_INTEGER, prev, tag,  &
      &                  comm, ierr )
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Test(requests(2), flag, status, ierr)
         end do
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'test', errs )
!
         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
!
         call MPI_Start(requests(1), ierr)
         call MPI_Wait(requests(1), status, ierr)
!
         call MPI_Request_free(requests(1), ierr)
      end if
!
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(2), ierr)
!
      end
!
      subroutine test_pair_pssend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer outcount, indices(2)
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer statuses(MPI_STATUS_SIZE,2), requests(2)
      integer status(MPI_STATUS_SIZE)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Persistent Ssend and recv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 3789
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL, &
      &                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                   requests(1), ierr)
!
      if (rank .eq. 0) then
!
         call MPI_Ssend_init(send_buf, count, MPI_REAL, next, tag, &
      &                       comm, requests(2), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Startall(2, requests, ierr)
!
         index = -1
         do while (index .ne. 1)
            call MPI_Testsome(2, requests, outcount, &
      &                        indices, statuses, ierr)
            do i = 1,outcount
               if (indices(i) .eq. 1) then
                  call msg_check( recv_buf, next, tag, count, &
      &                 statuses(1,i), TEST_SIZE, 'testsome', errs )
                  index = 1
               end if
            end do
         end do
!
         call MPI_Request_free(requests(2), ierr)
!
      else if (prev .eq. 0) then
!
         call MPI_Ssend_init(send_buf, count, MPI_REAL, prev, tag, &
      &                       comm, requests(2), ierr)
!
         call MPI_Start(requests(1), ierr)
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testany(1, requests(1), index, flag, &
      &                       statuses(1,1), ierr)
         end do
         call msg_check( recv_buf, prev, tag, count, statuses(1,1), &
      &           TEST_SIZE, 'testany', errs )

         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
!
         call MPI_Start(requests(2), ierr)
         call MPI_Wait(requests(2), status, ierr)
!
         call MPI_Request_free(requests(2), ierr)
!
      end if
!
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(1), ierr)
!
      end
!
      subroutine test_pair_sendrecv( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Sendrecv'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 4123
      count = TEST_SIZE / 5

      call clear_test_data(recv_buf,TEST_SIZE)

      if (rank .eq. 0) then

         call init_test_data(send_buf,TEST_SIZE)

         call MPI_Sendrecv(send_buf, count, MPI_REAL, next, tag, &
      &                     recv_buf, count, MPI_REAL, next, tag, &
      &                     comm, status, ierr)

         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'sendrecv', errs )

      else if (prev .eq. 0) then

         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'recv/send', errs )

         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, &
      &                 comm, ierr)
      end if
!
      end
!
      subroutine test_pair_sendrecvrepl( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Sendrecv replace'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 4456
      count = TEST_SIZE / 3

      if (rank .eq. 0) then
!
         call init_test_data(recv_buf, TEST_SIZE)
!
         do 11 i = count+1,TEST_SIZE
            recv_buf(i) = 0.0
 11      continue
!
         call MPI_Sendrecv_replace(recv_buf, count, MPI_REAL, &
      &                             next, tag, next, tag, &
      &                             comm, status, ierr)

         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'sendrecvreplace', errs )

      else if (prev .eq. 0) then

         call clear_test_data(recv_buf,TEST_SIZE)

         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'recv/send for replace', errs )

         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, &
      &                 comm, ierr)
      end if
!
      end
!
!------------------------------------------------------------------------------
!
!  Check for correct source, tag, count, and data in test message.
!
!------------------------------------------------------------------------------
      subroutine msg_check( recv_buf, source, tag, count, status, n,  &
      &                      name, errs )
      use mpi
      integer n, errs
      real    recv_buf(n)
      integer source, tag, count, rank, status(MPI_STATUS_SIZE)
      character*(*) name
      logical foundError

      integer ierr, recv_src, recv_tag, recv_count

      foundError = .false.
      recv_src = status(MPI_SOURCE)
      recv_tag = status(MPI_TAG)
      call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )
      call MPI_Get_count(status, MPI_REAL, recv_count, ierr)

      if (recv_src .ne. source) then
         print *, '[', rank, '] Unexpected source:', recv_src,  source, &
      &            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_tag .ne. tag) then
         print *, '[', rank, '] Unexpected tag:', recv_tag, tag, ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_count .ne. count) then
         print *, '[', rank, '] Unexpected count:', recv_count, count, &
      &            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      call verify_test_data(recv_buf, count, n, name, errs )

      end
!------------------------------------------------------------------------------
!
!  Check that requests have been set to null
!
!------------------------------------------------------------------------------
      subroutine rq_check( requests, n, msg )
      use mpi
      integer n, requests(n)
      character*(*) msg
      integer i
!
      do 10 i=1, n
         if (requests(i) .ne. MPI_REQUEST_NULL) then
            print *, 'Nonnull request in ', msg
         endif
 10   continue
!
      end
!------------------------------------------------------------------------------
!
!  Initialize test data buffer with integral sequence.
!
!------------------------------------------------------------------------------
      subroutine init_test_data(buf,n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = REAL(i)
 10    continue
      end

!------------------------------------------------------------------------------
!
!  Clear test data buffer
!
!------------------------------------------------------------------------------
      subroutine clear_test_data(buf, n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = 0.
 10   continue

      end

!------------------------------------------------------------------------------
!
!  Verify test data buffer
!
!------------------------------------------------------------------------------
      subroutine verify_test_data( buf, count, n, name, errs )
      use mpi
      integer n, errs
      real buf(n)
      character *(*) name
      integer count, ierr, i
!
      do 10 i = 1, count
         if (buf(i) .ne. REAL(i)) then
            print 100, buf(i), i, count, name
            errs = errs + 1
         endif
 10   continue
!
      do 20 i = count + 1, n
         if (buf(i) .ne. 0.) then
            print 100, buf(i), i, n, name
            errs = errs + 1
         endif
 20   continue
!
100   format('Invalid data', f6.1, ' at ', i4, ' of ', i4, ' in ', a)
!
      end
!
!    This routine is used to prevent the compiler from deallocating the
!    array "a", which may happen in some of the tests (see the text in
!    the MPI standard about why this may be a problem in valid Fortran
!    codes).  Without this, for example, tests fail with the Cray ftn
!    compiler.
!
      subroutine dummyRef( a, n, ie )
      integer n, ie
      real    a(n)
! This condition will never be true, but the compile won't know that
      if (ie .eq. -1) then
          print *, a(n)
      endif
      return
      end
