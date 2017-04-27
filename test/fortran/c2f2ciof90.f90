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

! This file created from test/mpi/f77/io/c2f2ciof.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Test just the MPI-IO FILE object
      program main
      use mpi
      integer errs, toterrs, ierr
      integer wrank
      integer wgroup
      integer fsize, frank
      integer comm, file, group, result
      integer c2ffile

      errs = 0

      call mpi_init( ierr )

      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      call  mpi_comm_group( MPI_COMM_WORLD, wgroup, ierr )

      call mpi_file_open( MPI_COMM_WORLD, "temp", MPI_MODE_RDWR + &
      &     MPI_MODE_DELETE_ON_CLOSE + MPI_MODE_CREATE, MPI_INFO_NULL, &
      &     file, ierr ) 
      if (ierr .ne. 0) then
         errs = errs + 1
      else
         errs = errs + c2ffile( file )
         call mpi_file_close( file, ierr )
      endif

      call f2cfile( file )
!     name is temp, in comm world, no info provided
      call mpi_file_get_group( file, group, ierr )
      call mpi_group_compare( group, wgroup, result, ierr )
      if (result .ne. MPI_IDENT) then
          errs = errs + 1
          print *, "Group of file not the group of comm_world"
      endif
      call mpi_group_free( group, ierr )
      call mpi_group_free( wgroup, ierr )
      call mpi_file_close( file, ierr )
!
! Summarize the errors
!
      call mpi_allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, &
      &     MPI_COMM_WORLD, ierr )
      if (wrank .eq. 0) then
         if (toterrs .eq. 0) then
            print *, ' No Errors'
         else
            print *, ' Found ', toterrs, ' errors'
         endif
      endif

      call mpi_finalize( ierr )
      end
      
