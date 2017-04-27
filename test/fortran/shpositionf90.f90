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

! This file created from test/mpi/f77/io/shpositionf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi
        integer comm, fh, r, s, i
        integer fileintsize
        integer errs, err, ierr
        character *(100) filename
        integer (kind=MPI_OFFSET_KIND) offset

        integer (kind=MPI_ADDRESS_KIND) aint


        errs = 0
        call MTest_Init( ierr )

        filename = "iotest.txt"
        comm = MPI_COMM_WORLD
        call mpi_comm_size( comm, s, ierr )
        call mpi_comm_rank( comm, r, ierr )
! Try writing the file, then check it
        call mpi_file_open( comm, filename, MPI_MODE_RDWR +  &
      &                      MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
        if (ierr .ne. MPI_SUCCESS) then
           errs = errs + 1
           if (errs .le. 10) then
              call MTestPrintError( ierr )
           endif
        endif
!
! Get the size of an INTEGER in the file
        call mpi_file_get_type_extent( fh, MPI_INTEGER, aint, ierr )
        fileintsize = aint
!
! We let each process write in turn, getting the position after each 
! write
        do i=1, s
           if (i .eq. r + 1) then
              call mpi_file_write_shared( fh, i, 1, MPI_INTEGER,  &
      &            MPI_STATUS_IGNORE, ierr )
           if (ierr .ne. MPI_SUCCESS) then
              errs = errs + 1
              if (errs .le. 10) then
                 call MTestPrintError( ierr )
              endif
           endif
           endif
           call mpi_barrier( comm, ierr )
           call mpi_file_get_position_shared( fh, offset, ierr )
           if (offset .ne. fileintsize * i) then
              errs = errs + 1
              print *, r, ' Shared position is ', offset,' should be ', &
      &                 fileintsize * i
           endif
           call mpi_barrier( comm, ierr )
        enddo
        call mpi_file_close( fh, ierr )
        if (r .eq. 0) then
            call mpi_file_delete( filename, MPI_INFO_NULL, ierr )
        endif
        if (ierr .ne. MPI_SUCCESS) then
           errs = errs + 1
           if (errs .le. 10) then
              call MTestPrintError( ierr )
           endif
        endif
!
        call MTest_Finalize( errs )
        call mpi_finalize( ierr )
        end
