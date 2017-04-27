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

! This file created from test/mpi/f77/comm/commnamef.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer errs, ierr
      integer comm(4), i, rlen, ln
      integer ncomm
      character*(MPI_MAX_OBJECT_NAME) inname(4), cname
      logical MTestGetIntracomm

      errs = 0
      call mtest_init( ierr )
      
! Test the predefined communicators
      do ln=1,MPI_MAX_OBJECT_NAME
         cname(ln:ln) = 'X'
      enddo
      call mpi_comm_get_name( MPI_COMM_WORLD, cname, rlen, ierr )
      do ln=MPI_MAX_OBJECT_NAME,1,-1
         if (cname(ln:ln) .ne. ' ') then
            if (ln .ne. rlen) then
               errs = errs + 1
               print *, 'result len ', rlen,' not equal to actual len ', &
      &              ln
            endif
            goto 110
         endif
      enddo
      if (cname(1:rlen) .ne. 'MPI_COMM_WORLD') then
         errs = errs + 1
         print *, 'Did not get MPI_COMM_WORLD for world'
      endif
 110  continue
!
      do ln=1,MPI_MAX_OBJECT_NAME
         cname(ln:ln) = 'X'
      enddo
      call mpi_comm_get_name( MPI_COMM_SELF, cname, rlen, ierr )
      do ln=MPI_MAX_OBJECT_NAME,1,-1
         if (cname(ln:ln) .ne. ' ') then
            if (ln .ne. rlen) then
               errs = errs + 1
               print *, 'result len ', rlen,' not equal to actual len ', &
      &              ln
            endif
            goto 120
         endif
      enddo
      if (cname(1:rlen) .ne. 'MPI_COMM_SELF') then
         errs = errs + 1
         print *, 'Did not get MPI_COMM_SELF for world'
      endif
 120  continue
!
      do i = 1, 4
         if (MTestGetIntracomm( comm(i), 1, .true. )) then
            ncomm = i
            write( inname(i), '(a,i1)') 'myname',i
            call mpi_comm_set_name( comm(i), inname(i), ierr )
         else
            goto 130
         endif
      enddo
 130   continue
!
!     Now test them all
      do i=1, ncomm
         call mpi_comm_get_name( comm(i), cname, rlen, ierr )
         if (inname(i) .ne. cname) then
            errs = errs + 1
            print *, ' Expected ', inname(i), ' got ', cname
         endif
         call MTestFreeComm( comm(i) )
      enddo
!      
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
