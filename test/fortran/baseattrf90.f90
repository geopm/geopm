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

! This file created from test/mpi/f77/attr/baseattrf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer value, commsize
      logical flag
      integer ierr, errs

      errs = 0
      call mpi_init( ierr )

      call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )
      call mpi_attr_get( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, value, flag &
      &     , ierr)
      ! MPI_UNIVERSE_SIZE need not be set
      if (flag) then
         if (value .lt. commsize) then
            print *, "MPI_UNIVERSE_SIZE is ", value, " less than world " &
      &           , commsize
            errs = errs + 1
         endif
      endif

      call mpi_attr_get( MPI_COMM_WORLD, MPI_LASTUSEDCODE, value, flag, &
      &     ierr )
      ! Last used code must be defined and >= MPI_ERR_LASTCODE
      if (flag) then
         if (value .lt. MPI_ERR_LASTCODE) then
            errs = errs + 1
            print *, "MPI_LASTUSEDCODE points to an integer &
      &           (", value, ") smaller than MPI_ERR_LASTCODE (", &
      &           MPI_ERR_LASTCODE, ")"
         endif
      else
         errs = errs + 1
         print *, "MPI_LASTUSECODE is not defined"
      endif

      call mpi_attr_get( MPI_COMM_WORLD, MPI_APPNUM, value, flag, ierr )
      ! appnum need not be set
      if (flag) then
         if (value .lt. 0) then
            errs = errs + 1
            print *, "MPI_APPNUM is defined as ", value, &
      &           " but must be nonnegative"
         endif
      endif

      ! Check for errors
      if (errs .eq. 0) then
         print *, " No Errors"
      else
         print *, " Found ", errs, " errors"
      endif

      call MPI_Finalize( ierr )

      end

