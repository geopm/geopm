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

! This file created from test/mpi/f77/datatype/allctypesf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer atype, ierr
!
      call mtest_init(ierr)
      call mpi_comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN,  &
      &                              ierr )
!
!     Check that all Ctypes are available in Fortran (MPI 2.1, p 483, line 46)
!
       call checkdtype( MPI_CHAR, "MPI_CHAR", ierr )
       call checkdtype( MPI_SIGNED_CHAR, "MPI_SIGNED_CHAR", ierr )
       call checkdtype( MPI_UNSIGNED_CHAR, "MPI_UNSIGNED_CHAR", ierr )
       call checkdtype( MPI_BYTE, "MPI_BYTE", ierr )
       call checkdtype( MPI_WCHAR, "MPI_WCHAR", ierr )
       call checkdtype( MPI_SHORT, "MPI_SHORT", ierr )
       call checkdtype( MPI_UNSIGNED_SHORT, "MPI_UNSIGNED_SHORT", ierr )
       call checkdtype( MPI_INT, "MPI_INT", ierr )
       call checkdtype( MPI_UNSIGNED, "MPI_UNSIGNED", ierr )
       call checkdtype( MPI_LONG, "MPI_LONG", ierr )
       call checkdtype( MPI_UNSIGNED_LONG, "MPI_UNSIGNED_LONG", ierr )
       call checkdtype( MPI_FLOAT, "MPI_FLOAT", ierr )
       call checkdtype( MPI_DOUBLE, "MPI_DOUBLE", ierr )
       if (MPI_LONG_DOUBLE .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_LONG_DOUBLE, "MPI_LONG_DOUBLE", ierr )
       endif
       if (MPI_LONG_LONG_INT .ne. MPI_DATATYPE_NULL) then
         call checkdtype2( MPI_LONG_LONG_INT, "MPI_LONG_LONG_INT",  &
      &                     "MPI_LONG_LONG", ierr )
       endif
       if (MPI_UNSIGNED_LONG_LONG .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_UNSIGNED_LONG_LONG,  &
      &                    "MPI_UNSIGNED_LONG_LONG", ierr )
       endif
       if (MPI_LONG_LONG .ne. MPI_DATATYPE_NULL) then
         call checkdtype2( MPI_LONG_LONG, "MPI_LONG_LONG",  &
      &                     "MPI_LONG_LONG_INT", ierr )
       endif
       call checkdtype( MPI_PACKED, "MPI_PACKED", ierr )
       call checkdtype( MPI_LB, "MPI_LB", ierr )
       call checkdtype( MPI_UB, "MPI_UB", ierr )
       call checkdtype( MPI_FLOAT_INT, "MPI_FLOAT_INT", ierr )
       call checkdtype( MPI_DOUBLE_INT, "MPI_DOUBLE_INT", ierr )
       call checkdtype( MPI_LONG_INT, "MPI_LONG_INT", ierr )
       call checkdtype( MPI_SHORT_INT, "MPI_SHORT_INT", ierr )
       call checkdtype( MPI_2INT, "MPI_2INT", ierr )
       if (MPI_LONG_DOUBLE_INT .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT", &
      &                    ierr)
       endif
!
!     Check that all Ctypes are available in Fortran (MPI 2.2)
!     Note that because of implicit declarations in Fortran, this
!     code should compile even with pre MPI 2.2 implementations.
!
       if (MPI_VERSION .gt. 2 .or. (MPI_VERSION .eq. 2 .and.  &
      &      MPI_SUBVERSION .ge. 2)) then
          call checkdtype( MPI_INT8_T, "MPI_INT8_T", ierr )
          call checkdtype( MPI_INT16_T, "MPI_INT16_T", ierr )
          call checkdtype( MPI_INT32_T, "MPI_INT32_T", ierr )
          call checkdtype( MPI_INT64_T, "MPI_INT64_T", ierr )
          call checkdtype( MPI_UINT8_T, "MPI_UINT8_T", ierr )
          call checkdtype( MPI_UINT16_T, "MPI_UINT16_T", ierr )
          call checkdtype( MPI_UINT32_T, "MPI_UINT32_T", ierr )
          call checkdtype( MPI_UINT64_T, "MPI_UINT64_T", ierr )
! other C99 types
          call checkdtype( MPI_C_BOOL, "MPI_C_BOOL", ierr )
          call checkdtype( MPI_C_FLOAT_COMPLEX, "MPI_C_FLOAT_COMPLEX", &
      &                     ierr)
          call checkdtype2( MPI_C_COMPLEX, "MPI_C_COMPLEX",  &
      &                      "MPI_C_FLOAT_COMPLEX", ierr )
          call checkdtype( MPI_C_DOUBLE_COMPLEX, "MPI_C_DOUBLE_COMPLEX",  &
      &                     ierr )
          if (MPI_C_LONG_DOUBLE_COMPLEX .ne. MPI_DATATYPE_NULL) then
            call checkdtype( MPI_C_LONG_DOUBLE_COMPLEX,  &
      &                       "MPI_C_LONG_DOUBLE_COMPLEX", ierr )
          endif
! address/offset types
          call checkdtype( MPI_AINT, "MPI_AINT", ierr )
          call checkdtype( MPI_OFFSET, "MPI_OFFSET", ierr )
       endif
!
       call mtest_finalize( ierr )
       call MPI_Finalize( ierr )
       end
!
! Check name of datatype
      subroutine CheckDtype( intype, name, ierr )
      use mpi
      integer intype, ierr
      character *(*) name
      integer ir, rlen
      character *(MPI_MAX_OBJECT_NAME) outname
!
      outname = ""
      call MPI_TYPE_GET_NAME( intype, outname, rlen, ir )
      if (ir .ne. MPI_SUCCESS) then
         print *, " Datatype ", name, " not available in Fortran"
         ierr = ierr + 1
      else
         if (outname .ne. name) then
            print *, " For datatype ", name, " found name ", &
      &           outname(1:rlen)
            ierr = ierr + 1
         endif
      endif

      return
      end
!
! Check name of datatype (allows alias)
      subroutine CheckDtype2( intype, name, name2, ierr )
      use mpi
      integer intype, ierr
      character *(*) name, name2
      integer ir, rlen
      character *(MPI_MAX_OBJECT_NAME) outname
!
      outname = ""
      call MPI_TYPE_GET_NAME( intype, outname, rlen, ir )
      if (ir .ne. MPI_SUCCESS) then
         print *, " Datatype ", name, " not available in Fortran"
         ierr = ierr + 1
      else
         if (outname .ne. name .and. outname .ne. name2) then
            print *, " For datatype ", name, " found name ", &
      &           outname(1:rlen)
            ierr = ierr + 1
         endif
      endif

      return
      end
