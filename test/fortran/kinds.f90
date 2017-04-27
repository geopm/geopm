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

! -*- Mode: Fortran; -*-
!
!  (C) 2011 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program tests that all of the integer kinds defined in MPI 2.2 are
! available.
!
  program main
  use mpi
  integer (kind=MPI_ADDRESS_KIND) aint, taint
  integer (kind=MPI_OFFSET_KIND) oint, toint
  integer (kind=MPI_INTEGER_KIND) iint, tiint
  integer s(MPI_STATUS_SIZE)
  integer i, wsize, wrank, ierr, errs
!
  errs = 0
!
  call MTEST_INIT(ierr)
  call MPI_COMM_SIZE(MPI_COMM_WORLD,wsize,ierr)
  call MPI_COMM_RANK(MPI_COMM_WORLD,wrank,ierr)
  if (wsize .lt. 2) then
     print *, "This test requires at least 2 processes"
     call MPI_ABORT( MPI_COMM_WORLD, 1, ierr )
  endif
!
! Some compilers (e.g., gfortran) will issue an error if, at compile time,
! an assignment would cause overflow, even if appropriated guarded.  To
! avoid this problem, we must compute the value in the integer (the
! code here is simple; there are faster fixes for this but this is easy
  if (wrank .eq. 0) then
     if (range(aint) .ge. 10) then
        aint = 1
        do i=1, range(aint)-1
           aint = aint * 10
        enddo
        aint = aint - 1
     else
        aint = 12345678
     endif
     if (range(oint) .ge. 10) then
        oint = 1
        do i=1, range(oint)-1
           oint = oint * 10
        enddo
        oint = oint - 1
     else
        oint = 12345678
     endif
     if (range(iint) .ge. 10) then
        iint = 1
        do i=1, range(iint)-1
           iint = iint * 10
        enddo
        iint = iint - 1
     else
        iint = 12345678
     endif
     call MPI_SEND( aint, 1, MPI_AINT, 1, 0, MPI_COMM_WORLD, ierr )
     call MPI_SEND( oint, 1, MPI_OFFSET, 1, 1, MPI_COMM_WORLD, ierr )
     call MPI_SEND( iint, 1, MPI_INTEGER, 1, 2, MPI_COMM_WORLD, ierr )
!
  else if (wrank .eq. 1) then
     if (range(taint) .ge. 10) then
        taint = 1
        do i=1, range(taint)-1
           taint = taint * 10
        enddo
        taint = taint - 1
     else
        taint = 12345678
     endif
     if (range(toint) .ge. 10) then
        toint = 1
        do i=1, range(toint)-1
           toint = toint * 10
        enddo
        toint = toint - 1
     else
        toint = 12345678
     endif
     if (range(tiint) .ge. 10) then
        tiint = 1
        do i=1, range(tiint)-1
           tiint = tiint * 10
        enddo
        tiint = tiint - 1
     else
        tiint = 12345678
     endif
     call MPI_RECV( aint, 1, MPI_AINT, 0, 0, MPI_COMM_WORLD, s, ierr )
     if (taint .ne. aint) then
        print *, "Address-sized int not correctly transfered"
        print *, "Value should be ", taint, " but is ", aint
        errs = errs + 1
     endif
     call MPI_RECV( oint, 1, MPI_OFFSET, 0, 1, MPI_COMM_WORLD, s, ierr )
     if (toint .ne. oint) then
        print *, "Offset-sized int not correctly transfered"
        print *, "Value should be ", toint, " but is ", oint
        errs = errs + 1
     endif
     call MPI_RECV( iint, 1, MPI_INTEGER, 0, 2, MPI_COMM_WORLD, s, ierr )
     if (tiint .ne. iint) then
        print *, "Integer (by kind) not correctly transfered"
        print *, "Value should be ", tiint, " but is ", iint
        errs = errs + 1
     endif
!
  endif
!
  call MTEST_FINALIZE(errs)
  call MPI_FINALIZE(ierr)

  end
