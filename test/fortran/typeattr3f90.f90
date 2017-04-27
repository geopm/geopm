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

! This file created from test/mpi/f77/attr/typeattr3f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!  This tests the null copy function (returns flag false; thus the
!  attribute should not be propagated to a dup'ed communicator
!  This is much like the test in typeattr2f
!
      program main
      use mpi
      integer errs, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer type1, type2
      integer keyval
      logical flag
!
! The only difference between the MPI-2 and MPI-1 attribute caching
! routines in Fortran is that the take an address-sized integer
! instead of a simple integer.  These still are not pointers,
! so the values are still just integers.
!
      errs      = 0
      call mtest_init( ierr )
      type1 = MPI_INTEGER
!
      extrastate = 1001
      call mpi_type_create_keyval( MPI_TYPE_NULL_COPY_FN,  &
      &                             MPI_TYPE_NULL_DELETE_FN, keyval,  &
      &                             extrastate, ierr )
      flag = .true.
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' get attr returned true when no attr set'
      endif

! Test the null copy function
      valin = 5001
      call mpi_type_set_attr( type1, keyval, valin, ierr )
      call mpi_type_dup( type1, type2, ierr )
! Because we set NULL_COPY_FN, the attribute should not
! appear on the dup'ed communicator
      flag = .false.
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (valout .ne. 5001) then
         errs = errs + 1
         print *, 'Unexpected output value in type ', valout
      endif
      flag = .true.
      call mpi_type_get_attr( type2, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Attribute incorrectly present on dup datatype'
      endif
! Test the delete function
      call mpi_type_free( type2, ierr )
!
! Test the attr delete function
      call mpi_type_dup( type1, type2, ierr )
      valin      = 6001
      extrastate = 1001
      call mpi_type_set_attr( type2, keyval, valin, ierr )
      call mpi_type_delete_attr( type2, keyval, ierr )
      flag = .true.
      call mpi_type_get_attr( type2, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif
      call mpi_type_free( type2, ierr )
!
      ierr = -1
      call mpi_type_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
