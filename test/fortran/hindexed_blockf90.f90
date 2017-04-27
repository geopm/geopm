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

! This file created from test/mpi/f77/datatype/hindexed_blockf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer errs, ierr, i, intsize
      integer type1, type2, type3, type4, type5
      integer max_asizev
      parameter (max_asizev = 10)
      integer (kind=MPI_ADDRESS_KIND) aint, aintv(max_asizev)

      integer blocklens(max_asizev), dtypes(max_asizev)
      integer displs(max_asizev)
      integer recvbuf(6*max_asizev)
      integer sendbuf(max_asizev), status(MPI_STATUS_SIZE)
      integer rank, size

      errs = 0

      call mtest_init( ierr )

      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
!
      call mpi_type_size( MPI_INTEGER, intsize, ierr )
!
      aintv(1) = 0
      aintv(2) = 3 * intsize
      call mpi_type_create_resized( MPI_INTEGER, aintv(1), aintv(2), &
      &                              type1, ierr )
      call mpi_type_commit( type1, ierr )
      aintv(1) = -1
      aintv(2) = -1
      call mpi_type_get_extent( type1, aintv(1), aintv(2), ierr )
      if (aintv(1) .ne. 0) then
         errs = errs + 1
         print *, 'Did not get expected lb'
      endif
      if (aintv(2) .ne. 3*intsize) then
         errs = errs + 1
         print *, 'Did not get expected extent'
      endif
      aintv(1) = -1
      aintv(2) = -1
      call mpi_type_get_true_extent( type1, aintv(1), aintv(2), ierr )
      if (aintv(1) .ne. 0) then
         errs = errs + 1
         print *, 'Did not get expected true lb'
      endif
      if (aintv(2) .ne. intsize) then
         errs = errs + 1
         print *, 'Did not get expected true extent (', aintv(2), ') ', &
      &     ' expected ', intsize
      endif
!
      do i=1,10
         blocklens(i) = 1
         aintv(i)    = (i-1) * 3 * intsize
      enddo
      call mpi_type_create_hindexed( 10, blocklens, aintv, &
      &                               MPI_INTEGER, type2, ierr )
      call mpi_type_commit( type2, ierr )
!
      aint = 3 * intsize
      call mpi_type_create_hvector( 10, 1, aint, MPI_INTEGER, type3, &
      &                              ierr )
      call mpi_type_commit( type3, ierr )
!
      do i=1,10
         blocklens(i) = 1
         dtypes(i)    = MPI_INTEGER
         aintv(i)    = (i-1) * 3 * intsize
      enddo
      call mpi_type_create_struct( 10, blocklens, aintv, dtypes, &
      &                             type4, ierr )
      call mpi_type_commit( type4, ierr )

      call mpi_type_get_extent(MPI_INTEGER, aintv(1), aint, ierr)
      do i=1,10
         aintv(i)    = (i-1) * 3 * aint
      enddo
      call mpi_type_create_hindexed_block( 10, 1, aintv, &
      &                               MPI_INTEGER, type5, ierr )
      call mpi_type_commit( type5, ierr )
!
! Using each time, send and receive using these types
      do i=1, max_asizev*3
         recvbuf(i) = -1
      enddo
      do i=1, max_asizev
         sendbuf(i) = i
      enddo
      call mpi_sendrecv( sendbuf, max_asizev, MPI_INTEGER, rank, 0, &
      &                   recvbuf, max_asizev, type1, rank, 0, &
      &                   MPI_COMM_WORLD, status, ierr )
      do i=1, max_asizev
         if (recvbuf(1+(i-1)*3) .ne. i ) then
            errs = errs + 1
            print *, 'type1:', i, 'th element = ', recvbuf(1+(i-1)*3)
         endif
      enddo
!
      do i=1, max_asizev*3
         recvbuf(i) = -1
      enddo
      do i=1, max_asizev
         sendbuf(i) = i
      enddo
      call mpi_sendrecv( sendbuf, max_asizev, MPI_INTEGER, rank, 0, &
      &                   recvbuf, 1, type2, rank, 0, &
      &                   MPI_COMM_WORLD, status, ierr )
      do i=1, max_asizev
         if (recvbuf(1+(i-1)*3) .ne. i ) then
            errs = errs + 1
            print *, 'type2:', i, 'th element = ', recvbuf(1+(i-1)*3)
         endif
      enddo
!
      do i=1, max_asizev*3
         recvbuf(i) = -1
      enddo
      do i=1, max_asizev
         sendbuf(i) = i
      enddo
      call mpi_sendrecv( sendbuf, max_asizev, MPI_INTEGER, rank, 0, &
      &                   recvbuf, 1, type3, rank, 0, &
      &                   MPI_COMM_WORLD, status, ierr )
      do i=1, max_asizev
         if (recvbuf(1+(i-1)*3) .ne. i ) then
            errs = errs + 1
            print *, 'type3:', i, 'th element = ', recvbuf(1+(i-1)*3)
         endif
      enddo
!
      do i=1, max_asizev*3
         recvbuf(i) = -1
      enddo
      do i=1, max_asizev
         sendbuf(i) = i
      enddo
      call mpi_sendrecv( sendbuf, max_asizev, MPI_INTEGER, rank, 0, &
      &                   recvbuf, 1, type4, rank, 0, &
      &                   MPI_COMM_WORLD, status, ierr )
      do i=1, max_asizev
         if (recvbuf(1+(i-1)*3) .ne. i ) then
            errs = errs + 1
            print *, 'type4:', i, 'th element = ', recvbuf(1+(i-1)*3)
         endif
      enddo
!
      do i=1, max_asizev*3
         recvbuf(i) = -1
      enddo
      do i=1, max_asizev
         sendbuf(i) = i
      enddo
      call mpi_sendrecv( sendbuf, max_asizev, MPI_INTEGER, rank, 0, &
      &                   recvbuf, 1, type5, rank, 0, &
      &                   MPI_COMM_WORLD, status, ierr )
      do i=1, max_asizev
         if (recvbuf(1+(i-1)*3) .ne. i ) then
            errs = errs + 1
            print *, 'type5:', i, 'th element = ', recvbuf(1+(i-1)*3)
         endif
      enddo
!
      call mpi_type_free( type1, ierr )
      call mpi_type_free( type2, ierr )
      call mpi_type_free( type3, ierr )
      call mpi_type_free( type4, ierr )
      call mpi_type_free( type5, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
