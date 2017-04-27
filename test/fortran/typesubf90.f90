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

! This file created from test/mpi/f77/datatype/typesubf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer errs, ierr
      integer maxn, maxm
      parameter (maxn=10,maxm=15)
      integer fullsizes(2), subsizes(2), starts(2)
      integer fullarr(maxn,maxm),subarr(maxn-3,maxm-4)
      integer i,j, ssize
      integer newtype, size, rank, ans

      errs = 0
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
!
! Create a Fortran-style subarray
      fullsizes(1) = maxn
      fullsizes(2) = maxm
      subsizes(1)  = maxn - 3
      subsizes(2)  = maxm - 4
! starts are from zero, even in Fortran
      starts(1)    = 1
      starts(2)    = 2
! In Fortran 90 notation, the original array is
!    integer a(maxn,maxm)
! and the subarray is
!    a(1+1:(maxn-3) +(1+1)-1,2+1:(maxm-4)+(2+1)-1)
! i.e., a (start:(len + start - 1),...)
      call mpi_type_create_subarray( 2, fullsizes, subsizes, starts,  &
      &         MPI_ORDER_FORTRAN, MPI_INTEGER, newtype, ierr )
      call mpi_type_commit( newtype, ierr )
!
! Prefill the array
      do j=1, maxm
         do i=1, maxn
            fullarr(i,j) = (i-1) + (j-1) * maxn
         enddo
      enddo
      do j=1, subsizes(2)
         do i=1, subsizes(1)
            subarr(i,j) = -1
         enddo
      enddo
      ssize = subsizes(1)*subsizes(2)
      call mpi_sendrecv( fullarr, 1, newtype, rank, 0,  &
      &                   subarr, ssize, MPI_INTEGER, rank, 0,  &
      &                   MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr )
!
! Check the data
      do j=1, subsizes(2)
         do i=1, subsizes(1)
            ans = (i+starts(1)-1) + (j+starts(2)-1) * maxn
            if (subarr(i,j) .ne. ans) then
               errs = errs + 1
               if (errs .le. 10) then
                  print *, rank, 'subarr(',i,',',j,') = ', subarr(i,j)
               endif
            endif
         enddo
      enddo

      call mpi_type_free( newtype, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
