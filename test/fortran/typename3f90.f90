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

! This file created from test/mpi/f77/datatype/typename3f.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!
!  (C) 2012 by Argonne National Laboratory.
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

      call MPI_Type_get_name( MPI_AINT, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_AINT") then
           errs = errs + 1
           print *, "Expected MPI_AINT but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_OFFSET, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_OFFSET") then
           errs = errs + 1
           print *, "Expected MPI_OFFSET but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_COUNT, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_COUNT") then
           errs = errs + 1
           print *, "Expected MPI_COUNT but got "//name(1:namelen)
      endif

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
