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

! This file created from test/mpi/f77/ext/c2f2cf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer errs, toterrs, ierr
      integer wrank, wsize
      integer wgroup, info, req
      integer fsize, frank
      integer comm, group, type, op, errh, result
      integer c2fcomm, c2fgroup, c2ftype, c2finfo, c2frequest, &
      &     c2ferrhandler, c2fop
      character value*100
      logical   flag
      errs = 0

      call mpi_init( ierr )

!
! Test passing a Fortran MPI object to C
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      errs = errs + c2fcomm( MPI_COMM_WORLD )
      call mpi_comm_group( MPI_COMM_WORLD, wgroup, ierr )
      errs = errs + c2fgroup( wgroup )
      call mpi_group_free( wgroup, ierr )

      call mpi_info_create( info, ierr )
      call mpi_info_set( info, "host", "myname", ierr )
      call mpi_info_set( info, "wdir", "/rdir/foo", ierr )
      errs = errs + c2finfo( info )
      call mpi_info_free( info, ierr )

      errs = errs + c2ftype( MPI_INTEGER )

      call mpi_irecv( 0, 0, MPI_INTEGER, MPI_ANY_SOURCE, MPI_ANY_TAG, &
      &     MPI_COMM_WORLD, req, ierr )
      call mpi_cancel( req, ierr )
      errs = errs + c2frequest( req )
      call mpi_wait( req, MPI_STATUS_IGNORE, ierr )

      errs = errs + c2ferrhandler( MPI_ERRORS_RETURN )

      errs = errs + c2fop( MPI_SUM )

!
! Test using a C routine to provide the Fortran handle
      call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )

      call f2ccomm( comm )
      call mpi_comm_size( comm, fsize, ierr )
      call mpi_comm_rank( comm, frank, ierr )
      if (fsize.ne.wsize .or. frank.ne.wrank) then
         errs = errs + 1
         print *, "Comm(fortran) has wrong size or rank"
      endif

      call f2cgroup( group )
      call mpi_group_size( group, fsize, ierr )
      call mpi_group_rank( group, frank, ierr )
      if (fsize.ne.wsize .or. frank.ne.wrank) then
         errs = errs + 1
         print *, "Group(fortran) has wrong size or rank"
      endif
      call mpi_group_free( group, ierr )

      call f2ctype( type )
      if (type .ne. MPI_INTEGER) then
         errs = errs + 1
         print *, "Datatype(fortran) is not MPI_INT"
      endif

      call f2cinfo( info )
      call mpi_info_get( info, "host", 100, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Info test for host returned false"
      else if (value .ne. "myname") then
         errs = errs + 1
         print *, "Info test for host returned ", value
      endif
      call mpi_info_get( info, "wdir", 100, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Info test for wdir returned false"
      else if (value .ne. "/rdir/foo") then
         errs = errs + 1
         print *, "Info test for wdir returned ", value
      endif
      call mpi_info_free( info, ierr )

      call f2cop( op )
      if (op .ne. MPI_SUM) then
          errs = errs + 1
          print *, "Fortran MPI_SUM not MPI_SUM in C"
      endif

      call f2cerrhandler( errh )
      if (errh .ne. MPI_ERRORS_RETURN) then
          errs = errs + 1
          print *,"Fortran MPI_ERRORS_RETURN not MPI_ERRORS_RETURN in C"
      endif
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

