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

! This file created from test/mpi/f77/io/setviewcurf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer (kind=MPI_OFFSET_KIND) offset

      integer errs, ierr, size, rank
      integer fh, comm, status(MPI_STATUS_SIZE)
      integer buf(1024)

      errs = 0
      call MTest_Init( ierr )

!    This test reads a header then sets the view to every "size" int,
!    using set view and current displacement.  The file is first written
!    using a combination of collective and ordered writes 
      
      comm = MPI_COMM_WORLD
      call MPI_File_open( comm, "test.ord", MPI_MODE_WRONLY + &
      &     MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintErrorMsg( "Open(1)", ierr )
      endif
      call MPI_Comm_size( comm, size, ierr )
      call MPI_Comm_rank( comm, rank, ierr )
      if (size .gt. 1024) then
         if (rank .eq. 0) then
            print *,  &
      &"This program must be run with no more than 1024 processes"
            call MPI_Abort( MPI_COMM_WORLD, 1, ierr )
         endif
      endif

      buf(1) = size
      call MPI_File_write_all( fh, buf, 1, MPI_INTEGER, status, ierr ) 
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintErrorMsg( "Write_all", ierr )
      endif
      call MPI_File_get_position( fh, offset, ierr )
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1 
         call MTestPrintErrorMsg( "Get_position", ierr )
      endif
      call MPI_File_seek_shared( fh, offset, MPI_SEEK_SET, ierr )
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1
         call MTestPrintErrorMsg( "Seek_shared", ierr )
      endif
      buf(1) = rank
      call MPI_File_write_ordered( fh, buf, 1, MPI_INTEGER, status,ierr)
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1
         call MTestPrintErrorMsg( "Write_ordered", ierr )
      endif
      call MPI_File_close( fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1 
         call MTestPrintErrorMsg( "Close(1)", ierr ) 
      endif

! Reopen the file as sequential 
      call MPI_File_open( comm, "test.ord", MPI_MODE_RDONLY + &
      &     MPI_MODE_SEQUENTIAL + MPI_MODE_DELETE_ON_CLOSE, &
      &     MPI_INFO_NULL, fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1 
         call MTestPrintErrorMsg( "Open(Read)", ierr )
      endif

      if (rank .eq. 0) then
         call MPI_File_read_shared( fh, buf, 1, MPI_INTEGER, status, &
      &        ierr ) 
         if (ierr .ne. MPI_SUCCESS) then 
            errs = errs + 1
            call MTestPrintErrorMsg( "Read_all", ierr )
         endif
         if (buf(1) .ne. size) then
           errs = errs + 1
           print *, "Unexpected value for the header = ", buf(1), &
      &          ", should be ", size 
        endif
      endif
      call MPI_Barrier( comm, ierr  )
!   All processes must provide the same file view for MODE_SEQUENTIAL
      call MPI_File_set_view( fh, MPI_DISPLACEMENT_CURRENT, MPI_INTEGER &
      &     ,MPI_INTEGER, "native", MPI_INFO_NULL, ierr )
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1 
         call MTestPrintErrorMsg( "Set_view", ierr )
      endif
      buf(1) = -1
      call MPI_File_read_ordered( fh, buf, 1, MPI_INTEGER, status, ierr &
      &     ) 
      if (ierr .ne. MPI_SUCCESS) then 
         errs = errs + 1
         call MTestPrintErrorMsg( "Read_all", ierr )
      endif
      if (buf(1) .ne. rank) then
         errs = errs + 1
         print *, rank, ": buf(1) = ", buf(1)
      endif

      call MPI_File_close( fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1 
         call MTestPrintErrorMsg( "Close(2)", ierr )
      endif

      call MTest_Finalize( errs )

      call MPI_Finalize( ierr )
      end

