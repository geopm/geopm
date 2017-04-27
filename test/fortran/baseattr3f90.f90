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

! -*- Mode: Fortran; -*-
!
!
! (C) 2012 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! This is a MPI-2 version of baseattr2f90.f90 which uses COMM_GET_ATTR 
! instead of ATTR_GET, using an address-sized integer instead of 
! an INTEGER.
        program main
        use mpi
        integer ierr, errs
        logical flag
        integer commsize, commrank
        integer (KIND=MPI_ADDRESS_KIND) value

        errs = 0
        call mpi_init( ierr )

        call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )
        call mpi_comm_rank( MPI_COMM_WORLD, commrank, ierr )

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_TAG_UB, value,   &
             & flag, ierr ) 
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get TAG_UB"
        else
           if (value .lt. 32767) then
              errs = errs + 1
              print *, "Got too-small value (", value, ") for TAG_UB" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_HOST, value, flag&
             &, ierr ) 
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get HOST"
        else 
           if ((value .lt. 0 .or. value .ge. commsize) .and. value .ne. &
      &          MPI_PROC_NULL) then 
              errs = errs + 1
              print *, "Got invalid value ", value, " for HOST"
           endif
        endif   

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_IO, value, flag,&
             & ierr ) 
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get IO"
        else
           if ((value .lt. 0 .or. value .ge. commsize) .and. value .ne. &
      &          MPI_ANY_SOURCE .and. value .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, "Got invalid value ", value, " for IO"
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL,&
             & value, flag, ierr ) 
        if (flag) then
!          Wtime need not be set
           if (value .lt.  0 .or. value .gt. 1) then 
              errs = errs + 1
              print *, "Invalid value for WTIME_IS_GLOBAL (got ", value, &
      &             ")" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_APPNUM, value,&
             & flag, ierr ) 
!     appnum need not be set
        if (flag) then
           if (value .lt. 0) then
              errs = errs + 1
              print *, "MPI_APPNUM is defined as ", value, &
      &             " but must be nonnegative" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE,&
             & value, flag, ierr ) 
!     MPI_UNIVERSE_SIZE need not be set
        if (flag) then
           if (value .lt. commsize) then
              errs = errs + 1
              print *, "MPI_UNIVERSE_SIZE = ", value, &
      &             ", less than comm world (", commsize, ")"
           endif
        endif
    
        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_LASTUSEDCODE,&
             & value, flag, ierr ) 
! Last used code must be defined and >= MPI_ERR_LASTCODE
        if (flag) then
           if (value .lt. MPI_ERR_LASTCODE) then
            errs = errs + 1
            print *, "MPI_LASTUSEDCODE points to an integer (", &
      &           MPI_ERR_LASTCODE, ") smaller than MPI_ERR_LASTCODE (", &
      &           value, ")"
            endif
         else 
            errs = errs + 1
            print *, "MPI_LASTUSECODE is not defined"
         endif

!     Check for errors
      if (errs .eq. 0) then
         print *, " No Errors"
      else
         print *, " Found ", errs, " errors"
      endif

      call MPI_Finalize( ierr )

      end
