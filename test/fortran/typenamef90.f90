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

! This file created from test/mpi/f77/datatype/typenamef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      character*(MPI_MAX_OBJECT_NAME) name
      integer namelen
      integer ierr, errs

      errs = 0

      call mtest_init( ierr )
!
! Check each Fortran datatype, including the size-specific ones
! See the C version (typename.c) for the relevant MPI sections

      call MPI_Type_get_name( MPI_COMPLEX, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_COMPLEX") then
           errs = errs + 1
           print *, "Expected MPI_COMPLEX but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_DOUBLE_COMPLEX, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_DOUBLE_COMPLEX") then
           errs = errs + 1
           print *, "Expected MPI_DOUBLE_COMPLEX but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_LOGICAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_LOGICAL") then
           errs = errs + 1
           print *, "Expected MPI_LOGICAL but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_REAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_REAL") then
           errs = errs + 1
           print *, "Expected MPI_REAL but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_DOUBLE_PRECISION, name, namelen, ierr)
      if (name(1:namelen) .ne. "MPI_DOUBLE_PRECISION") then
           errs = errs + 1
           print *, "Expected MPI_DOUBLE_PRECISION but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_INTEGER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_INTEGER") then
           errs = errs + 1
           print *, "Expected MPI_INTEGER but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_2INTEGER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_2INTEGER") then
           errs = errs + 1
           print *, "Expected MPI_2INTEGER but got "//name(1:namelen)
      endif

! 2COMPLEX was present only in MPI 1.0
!      call MPI_Type_get_name( MPI_2COMPLEX, name, namelen, ierr )
!      if (name(1:namelen) .ne. "MPI_2COMPLEX") then
!           errs = errs + 1
!           print *, "Expected MPI_2COMPLEX but got "//name(1:namelen)
!      endif
!
      call MPI_Type_get_name(MPI_2DOUBLE_PRECISION, name, namelen, ierr)
      if (name(1:namelen) .ne. "MPI_2DOUBLE_PRECISION") then
           errs = errs + 1
           print *, "Expected MPI_2DOUBLE_PRECISION but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_2REAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_2REAL") then
           errs = errs + 1
           print *, "Expected MPI_2REAL but got "//name(1:namelen)
      endif

! 2DOUBLE_COMPLEX isn't in MPI 2.1
!      call MPI_Type_get_name( MPI_2DOUBLE_COMPLEX, name, namelen, ierr )
!      if (name(1:namelen) .ne. "MPI_2DOUBLE_COMPLEX") then
!           errs = errs + 1
!           print *, "Expected MPI_2DOUBLE_COMPLEX but got "//
!     &          name(1:namelen)
!      endif

      call MPI_Type_get_name( MPI_CHARACTER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_CHARACTER") then
           errs = errs + 1
           print *, "Expected MPI_CHARACTER but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_BYTE, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_BYTE") then
           errs = errs + 1
           print *, "Expected MPI_BYTE but got "//name(1:namelen)
      endif

      if (MPI_REAL4 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL4, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL4") then
               errs = errs + 1
               print *, "Expected MPI_REAL4 but got "//name(1:namelen)
          endif
      endif

      if (MPI_REAL8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL8") then
               errs = errs + 1
               print *, "Expected MPI_REAL8 but got "//name(1:namelen)
          endif
      endif

      if (MPI_REAL16 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL16, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL16") then
               errs = errs + 1
               print *, "Expected MPI_REAL16 but got "//name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX8") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX8 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX16 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX16, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX16") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX16 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX32 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX32, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX32") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX32 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER1 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER1, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER1") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER1 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER2 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER2, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER2") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER2 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER4 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER4, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER4") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER4 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER8") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER8 but got "// &
      &              name(1:namelen)
          endif
      endif

! MPI_INTEGER16 is in MPI 2.1, but it is missing from most tables
! Some MPI implementations may not provide it
!      if (MPI_INTEGER16 .ne. MPI_DATATYPE_NULL) then
!          call MPI_Type_get_name( MPI_INTEGER16, name, namelen, ierr )
!          if (name(1:namelen) .ne. "MPI_INTEGER16") then
!               errs = errs + 1
!               print *, "Expected MPI_INTEGER16 but got "//
!     &              name(1:namelen)
!          endif
!      endif

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
