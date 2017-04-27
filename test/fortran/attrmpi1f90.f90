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

! This file created from test/mpi/f77/attr/attrmpi1f.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer value, wsize, wrank, extra, mykey
      integer rvalue, svalue, ncomm
      logical flag
      integer ierr, errs
!
      errs = 0
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
!
!     Simple attribute put and get
!
      call mpi_keyval_create( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &
      &     mykey, extra,ierr ) 
      call mpi_attr_get( MPI_COMM_WORLD, mykey, value, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, &
      &       "Did not get flag==.false. for attribute that was not set"
      endif
!
      value = 1234567
      svalue = value
      call mpi_attr_put( MPI_COMM_WORLD, mykey, value, ierr )
      value = -9876543
      call mpi_attr_get( MPI_COMM_WORLD, mykey, rvalue, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Did not find attribute after set"
      else
         if (rvalue .ne. svalue) then
            errs = errs + 1
            print *, "Attribute value ", rvalue, " should be ", svalue
         endif
      endif
      value = -123456
      svalue = value
      call mpi_attr_put( MPI_COMM_WORLD, mykey, value, ierr )
      value = 987654
      call mpi_attr_get( MPI_COMM_WORLD, mykey, rvalue, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Did not find attribute after set (neg)"
      else
         if (rvalue .ne. svalue) then
            errs = errs + 1
            print *, "Neg Attribute value ", rvalue," should be ",svalue
         endif
      endif
!      
      call mpi_keyval_free( mykey, ierr )
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
