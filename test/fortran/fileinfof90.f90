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

! This file created from test/mpi/f77/io/fileinfof.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer ierr, errs
      integer fh, info1, info2, rank
      logical flag
      character*(50) filename
      character*(MPI_MAX_INFO_KEY) mykey
      character*(MPI_MAX_INFO_VAL) myvalue

      errs = 0
      call mtest_init( ierr )

      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
!
! Open a simple file
      ierr = -1
      filename = "iotest.txt"
      call mpi_file_open( MPI_COMM_WORLD, filename, MPI_MODE_RDWR +  &
      &     MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
!
! Try to set one of the available info hints  
      call mpi_info_create( info1, ierr )
      call mpi_info_set( info1, "access_style",  &
      &                   "read_once,write_once", ierr )
      ierr = -1
      call mpi_file_set_info( fh, info1, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
      call mpi_info_free( info1, ierr )
      
      ierr = -1
      call mpi_file_get_info( fh, info2, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
      call mpi_info_get( info2, "filename", MPI_MAX_INFO_VAL,  &
      &                   myvalue, flag, ierr )
!
! An implementation isn't required to provide the filename (though
! a high-quality implementation should)
      if (flag) then
! If we find it, we must have the correct name
         if (myvalue(1:10) .ne. filename(1:10) .or. &
      &       myvalue(11:11) .ne. ' ') then
            errs = errs + 1
            print *, ' Returned wrong value for the filename'
         endif
      endif
      call mpi_info_free( info2, ierr )
!
      call mpi_file_close( fh, ierr )
      call mpi_barrier( MPI_COMM_WORLD, ierr )
      if (rank .eq. 0) then
         call mpi_file_delete( filename, MPI_INFO_NULL, ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
