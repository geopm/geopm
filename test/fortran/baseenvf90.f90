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

! This file created from test/mpi/f77/init/baseenvf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi
       integer ierr, provided, errs, rank, size
       integer iv, isubv, qprovided
       logical flag

       errs = 0
       flag = .true.
       call mpi_finalized( flag, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Returned true for finalized before init'
       endif
       flag = .true.
       call mpi_initialized( flag, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Return true for initialized before init'
       endif

       provided = -1
       call mpi_init_thread( MPI_THREAD_MULTIPLE, provided, ierr )

       if (provided .ne. MPI_THREAD_MULTIPLE .and.  &
      &     provided .ne. MPI_THREAD_SERIALIZED .and. &
      &     provided .ne. MPI_THREAD_FUNNELED .and. &
      &     provided .ne. MPI_THREAD_SINGLE) then
          errs = errs + 1
          print *, ' Unrecognized value for provided = ', provided
       endif

       iv    = -1
       isubv = -1
       call mpi_get_version( iv, isubv, ierr )
       if (iv .ne. MPI_VERSION .or. isubv .ne. MPI_SUBVERSION) then
          errs = errs + 1
          print *, 'Version in mpif.h and get_version do not agree'
          print *, 'Version in mpif.h is ', MPI_VERSION, '.',  &
      &              MPI_SUBVERSION
          print *, 'Version in get_version is ', iv, '.', isubv
       endif
       if (iv .lt. 1 .or. iv .gt. 3) then
          errs = errs + 1
          print *, 'Version of MPI is invalid (=', iv, ')'
       endif
       if (isubv.lt.0 .or. isubv.gt.2) then
          errs = errs + 1
          print *, 'Subversion of MPI is invalid (=', isubv, ')'
       endif

       call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
       call mpi_comm_size( MPI_COMM_WORLD, size, ierr )

       flag = .false.
       call mpi_is_thread_main( flag, ierr )
       if (.not.flag) then
          errs = errs + 1
          print *, 'is_thread_main returned false for main thread'
       endif

       call mpi_query_thread( qprovided, ierr )
       if (qprovided .ne. provided) then
          errs = errs + 1
          print *,'query thread and init thread disagree on'// &
      &           ' thread level'
       endif

       call mpi_finalize( ierr )
       flag = .false.
       call mpi_finalized( flag, ierr )
       if (.not. flag) then
          errs = errs + 1
          print *, 'finalized returned false after finalize'
       endif

       if (rank .eq. 0) then
          if (errs .eq. 0) then
             print *, ' No Errors'
          else
             print *, ' Found ', errs, ' errors'
          endif
       endif

       end
