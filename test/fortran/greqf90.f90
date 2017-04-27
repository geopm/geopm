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

! This file created from test/mpi/f77/pt2pt/greqf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      subroutine query_fn( extrastate, status, ierr )
      use mpi
      integer status(MPI_STATUS_SIZE), ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

!
!    set a default status
      status(MPI_SOURCE) = MPI_UNDEFINED
      status(MPI_TAG)    = MPI_UNDEFINED
      call mpi_status_set_cancelled( status, .false., ierr)
      call mpi_status_set_elements( status, MPI_BYTE, 0, ierr )
      ierr = MPI_SUCCESS
      end
!
      subroutine free_fn( extrastate, ierr )
      use mpi
      integer value, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer freefncall
      common /fnccalls/ freefncall
!
!   For testing purposes, the following print can be used to check whether
!   the free_fn is called
!      print *, 'Free_fn called'
!
      extrastate = extrastate - 1
!   The value returned by the free function is the error code
!   returned by the wait/test function 
      ierr = MPI_SUCCESS
      end
!
      subroutine cancel_fn( extrastate, complete, ierr )
      use mpi
      integer ierr
      logical complete
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val


      ierr = MPI_SUCCESS
      end
!
!
! This is a very simple test of generalized requests.  Normally, the
! MPI_Grequest_complete function would be called from another routine,
! often running in a separate thread.  This simple code allows us to
! check that requests can be created, tested, and waited on in the
! case where the request is complete before the wait is called.  
!
! Note that MPI did *not* define a routine that can be called within
! test or wait to advance the state of a generalized request.  
! Most uses of generalized requests will need to use a separate thread.
!
       program main
       use mpi
       integer errs, ierr
       logical flag
       integer status(MPI_STATUS_SIZE)
       integer request
       external query_fn, free_fn, cancel_fn
       integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

       integer freefncall
       common /fnccalls/ freefncall

       errs = 0
       freefncall = 0
       
       call MTest_Init( ierr )

       extrastate = 0
       call mpi_grequest_start( query_fn, free_fn, cancel_fn,  &
      &            extrastate, request, ierr )
       call mpi_test( request, flag, status, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Generalized request marked as complete'
       endif
       
       call mpi_grequest_complete( request, ierr )

       call MPI_Wait( request, status, ierr )

       extrastate = 1
       call mpi_grequest_start( query_fn, free_fn, cancel_fn,  &
      &                          extrastate, request, ierr )
       call mpi_grequest_complete( request, ierr )
       call mpi_wait( request, MPI_STATUS_IGNORE, ierr )
!       
!      The following routine may prevent an optimizing compiler from 
!      just remembering that extrastate was set in grequest_start
       call dummyupdate(extrastate)
       if (extrastate .ne. 0) then
          errs = errs + 1
          if (freefncall .eq. 0) then
              print *, 'Free routine not called'
          else 
              print *, 'Free routine did not update extra_data'
              print *, 'extrastate = ', extrastate
          endif
       endif
!
       call MTest_Finalize( errs )
       call mpi_finalize( ierr )
       end
!
