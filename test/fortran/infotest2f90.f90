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

! This file created from test/mpi/f77/info/infotest2f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer ierr, errs
      integer i1, i2
      integer nkeys, i, j, sumindex, vlen, ln, valuelen
      logical found, flag
      character*(MPI_MAX_INFO_KEY) keys(6)
      character*(MPI_MAX_INFO_VAL) values(6)
      character*(MPI_MAX_INFO_KEY) mykey
      character*(MPI_MAX_INFO_VAL) myvalue
!
      data keys/"Key1", "key2", "KeY3", "A Key With Blanks","See Below", &
      &          "last"/
      data values/"value 1", "value 2", "VaLue 3", "key=valu:3","false", &
      &            "no test"/
!
      errs = 0

      call mtest_init( ierr )

! Note that the MPI standard requires that leading an trailing blanks
! are stripped from keys and values (Section 4.10, The Info Object)
!
! First, create and initialize an info
      call mpi_info_create( i1, ierr )
      call mpi_info_set( i1, keys(1), values(1), ierr )
      call mpi_info_set( i1, keys(2), values(2), ierr )
      call mpi_info_set( i1, keys(3), values(3), ierr )
      call mpi_info_set( i1, keys(4), values(4), ierr )
      call mpi_info_set( i1, " See Below", values(5), ierr )
      call mpi_info_set( i1, keys(6), " no test ", ierr )
!
      call mpi_info_get_nkeys( i1, nkeys, ierr )
      if (nkeys .ne. 6) then
         print *, ' Number of keys should be 6, is ', nkeys
      endif
      sumindex = 0
      do i=1, nkeys
!        keys are number from 0 to n-1, even in Fortran (Section 4.10)
         call mpi_info_get_nthkey( i1, i-1, mykey, ierr )
         found = .false.
         do j=1, 6
            if (mykey .eq. keys(j)) then
               found = .true.
               sumindex = sumindex + j
               call mpi_info_get_valuelen( i1, mykey, vlen, flag, ierr )
               if (.not.flag) then
                  errs = errs + 1
                  print *, ' no value for key', mykey
               else
                  call mpi_info_get( i1, mykey, MPI_MAX_INFO_VAL, &
      &                               myvalue, flag, ierr )
                  if (myvalue .ne. values(j)) then
                     errs = errs + 1
                     print *, ' Value for ', mykey, ' not expected'
                  else
                     do ln=MPI_MAX_INFO_VAL,1,-1
                        if (myvalue(ln:ln) .ne. ' ') then
                           if (vlen .ne. ln) then
                              errs = errs + 1
                              print *, ' length is ', ln,  &
      &                          ' but valuelen gave ',  vlen,  &
      &                          ' for key ', mykey
                           endif
                           goto 100
                        endif
                     enddo
 100                 continue
                  endif
               endif
            endif
         enddo
         if (.not.found) then
            print *, i, 'th key ', mykey, ' not in list'
         endif
      enddo
      if (sumindex .ne. 21) then
         errs = errs + 1
         print *, ' Not all keys found'
      endif
!
! delete 2, then dup, then delete 2 more
      call mpi_info_delete( i1, keys(1), ierr )
      call mpi_info_delete( i1, keys(2), ierr )
      call mpi_info_dup( i1, i2, ierr )
      call mpi_info_delete( i1, keys(3), ierr )
!
! check the contents of i2
! valuelen does not signal an error for unknown keys; instead, sets
! flag to false
      do i=1,2
         flag = .true.
         call mpi_info_get_valuelen( i2, keys(i), valuelen, flag, ierr )
         if (flag) then
            errs = errs + 1
            print *, ' Found unexpected key ', keys(i)
         endif
         myvalue = 'A test'
         call mpi_info_get( i2, keys(i), MPI_MAX_INFO_VAL,  &
      &                      myvalue, flag, ierr )
         if (flag) then
            errs = errs + 1
            print *, ' Found unexpected key in MPI_Info_get ', keys(i)
         else
            if (myvalue .ne. 'A test') then
               errs = errs + 1
               print *, ' Returned value overwritten, is now ', myvalue
            endif
         endif

      enddo
      do i=3,6
         myvalue = ' '
         call mpi_info_get( i2, keys(i), MPI_MAX_INFO_VAL,  &
      &                      myvalue, flag, ierr )
         if (.not. flag) then
             errs = errs + 1
             print *, ' Did not find key ', keys(i)
         else
            if (myvalue .ne. values(i)) then
               errs = errs + 1
               print *, ' Found wrong value (', myvalue, ') for key ',  &
      &                  keys(i)
            endif
         endif
      enddo
!
!     Free info
      call mpi_info_free( i1, ierr )
      call mpi_info_free( i2, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
