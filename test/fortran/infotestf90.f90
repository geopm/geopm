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

! This file created from test/mpi/f77/info/infotestf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Simple info test
       program main
       use mpi
       integer i1, i2
       integer i, errs, ierr
       integer valuelen
       parameter (valuelen=64)
       character*(valuelen) value
       logical flag
!
       errs = 0

       call MTest_Init( ierr )

       call mpi_info_create( i1, ierr )
       call mpi_info_create( i2, ierr )

       call mpi_info_set( i1, "key1", "value1", ierr )
       call mpi_info_set( i2, "key2", "value2", ierr )

       call mpi_info_get( i1, "key2", valuelen, value, flag, ierr )
       if (flag) then
          print *, "Found key2 in info1"
          errs = errs + 1
       endif

       call MPI_Info_get( i1, "key1", 64, value, flag, ierr )
       if (.not. flag ) then
          print *, "Did not find key1 in info1"
          errs = errs + 1
       else
          if (value .ne. "value1") then
             print *, "Found wrong value (", value, "), expected value1"
             errs = errs + 1
          else
!     check for trailing blanks
             do i=7,valuelen
                if (value(i:i) .ne. " ") then
                   print *, "Found non blank in info value"
                   errs = errs + 1
                endif
             enddo
          endif
       endif

       call mpi_info_free( i1, ierr )
       call mpi_info_free( i2, ierr )

       call MTest_Finalize( errs )
       call MPI_Finalize( ierr )
       end
