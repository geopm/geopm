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

! This file created from test/mpi/f77/rma/winnamef.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer errs, ierr
      integer win, rlen, ln
      character*(MPI_MAX_OBJECT_NAME) cname
      integer buf(10)
      integer intsize
! Include addsize defines asize as an address-sized integer
      integer (kind=MPI_ADDRESS_KIND) asize

      logical found
!
      errs = 0
      call mtest_init( ierr )
!
! Create a window and get, set the names on it
!      
      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      asize = 10
      call mpi_win_create( buf, asize, intsize,  &
      &     MPI_INFO_NULL, MPI_COMM_WORLD, win, ierr )
!
!     Check that there is no name yet
      cname = 'XXXXXX'
      rlen  = -1
      call mpi_win_get_name( win, cname, rlen, ierr )
      if (rlen .ne. 0) then
         errs = errs + 1
         print *, ' Did not get empty name from new window'
      else if (cname(1:6) .ne. 'XXXXXX') then
         found = .false.
         do ln=MPI_MAX_OBJECT_NAME,1,-1
            if (cname(ln:ln) .ne. ' ') then
               found = .true.
            endif
         enddo
         if (found) then
            errs = errs + 1
            print *, ' Found a non-empty name'
         endif
      endif
!
! Now, set a name and check it
      call mpi_win_set_name( win, 'MyName', ierr )
      cname = 'XXXXXX'
      rlen = -1
      call mpi_win_get_name( win, cname, rlen, ierr )
      if (rlen .ne. 6) then
         errs = errs + 1
         print *, ' Expected 6, got ', rlen, ' for rlen'
         if (rlen .gt. 0 .and. rlen .lt. MPI_MAX_OBJECT_NAME) then
            print *, ' Cname = ', cname(1:rlen)
         endif
      else if (cname(1:6) .ne. 'MyName') then
         errs = errs + 1
         print *, ' Expected MyName, got ', cname(1:6)
      else
         found = .false.
         do ln=MPI_MAX_OBJECT_NAME,7,-1
            if (cname(ln:ln) .ne. ' ') then
               found = .true.
            endif
         enddo
         if (found) then
            errs = errs + 1
            print *, ' window name is not blank padded'
         endif
      endif
!      
      call mpi_win_free( win, ierr )
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
