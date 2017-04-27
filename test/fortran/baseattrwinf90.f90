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

! This file created from test/mpi/f77/rma/baseattrwinf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      logical flag
      integer ierr, errs
      integer base(1024)
      integer disp
      integer win
      integer commsize
! Include addsize defines asize as an address-sized integer
      integer (kind=MPI_ADDRESS_KIND) asize


      errs = 0
      
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )

! Create a window; then extract the values 
      asize    = 1024
      disp = 4
      call MPI_Win_create( base, asize, disp, MPI_INFO_NULL,  &
      &  MPI_COMM_WORLD, win, ierr )
!
! In order to check the base, we need an address-of function.
! We use MPI_Get_address, even though that isn't strictly correct
      call MPI_Win_get_attr( win, MPI_WIN_BASE, valout, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_BASE"
!
! There is no easy way to get the actual value of base to compare 
! against.  MPI_Address gives a value relative to MPI_BOTTOM, which 
! is different from 0 in Fortran (unless you can define MPI_BOTTOM
! as something like %pointer(0)).
!      else
!
!C For this Fortran 77 version, we use the older MPI_Address function
!         call MPI_Address( base, baseadd, ierr )
!         if (valout .ne. baseadd) then
!           errs = errs + 1
!           print *, "Got incorrect value for WIN_BASE (", valout, 
!     &             ", should be ", baseadd, ")"
!         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_SIZE, valout, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_SIZE"
      else
        if (valout .ne. asize) then
            errs = errs + 1
            print *, "Got incorrect value for WIN_SIZE (", valout,  &
      &        ", should be ", asize, ")"
         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_DISP_UNIT, valout, flag, ierr)
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_DISP_UNIT"
      else
         if (valout .ne. disp) then
            errs = errs + 1
            print *, "Got wrong value for WIN_DISP_UNIT (", valout,  &
      &               ", should be ", disp, ")"
         endif
      endif

      call MPI_Win_free( win, ierr )

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )

      end
