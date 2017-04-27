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

! This file created from test/mpi/f77/coll/reducelocalf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2009 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! Test Fortran MPI_Reduce_local with MPI_OP_SUM and with user-defined operation.
!
      subroutine user_op( invec, outvec, count, datatype )
      use mpi
      integer invec(*), outvec(*)
      integer count, datatype
      integer ii

      if (datatype .ne. MPI_INTEGER) then
         write(6,*) 'Invalid datatype passed to user_op()'
         return
      endif
      
      do ii=1, count
         outvec(ii) = invec(ii) * 2 + outvec(ii)
      enddo

      end

      program main
      use mpi
      integer max_buf_size
      parameter (max_buf_size=65000)
      integer vin(max_buf_size), vout(max_buf_size)
      external user_op
      integer ierr, errs
      integer count, myop
      integer ii
      
      errs = 0

      call mtest_init(ierr)

      count = 0
      do while (count .le. max_buf_size )
         do ii = 1,count
            vin(ii) = ii
            vout(ii) = ii
         enddo 
         call mpi_reduce_local( vin, vout, count, &
      &                          MPI_INTEGER, MPI_SUM, ierr )
!        Check if the result is correct
         do ii = 1,count
            if ( vin(ii) .ne. ii ) then
               errs = errs + 1
            endif
            if ( vout(ii) .ne. 2*ii ) then
               errs = errs + 1
            endif
         enddo 
         if ( count .gt. 0 ) then
            count = count + count
         else
            count = 1
         endif
      enddo

      call mpi_op_create( user_op, .false., myop, ierr )

      count = 0
      do while (count .le. max_buf_size) 
         do ii = 1, count
            vin(ii) = ii
            vout(ii) = ii
         enddo
         call mpi_reduce_local( vin, vout, count, &
      &                          MPI_INTEGER, myop, ierr )
!        Check if the result is correct
         do ii = 1, count
            if ( vin(ii) .ne. ii ) then
               errs = errs + 1
            endif
            if ( vout(ii) .ne. 3*ii ) then
               errs = errs + 1
            endif
         enddo
         if ( count .gt. 0 ) then
            count = count + count
         else
            count = 1
         endif
      enddo

      call mpi_op_free( myop, ierr )

      call mtest_finalize(errs)
      call mpi_finalize(ierr)

      end
