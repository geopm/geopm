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

! This file created from test/mpi/f77/topo/cartcrf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Test various combinations of periodic and non-periodic Cartesian
! communicators
!
      program main
      use mpi
      integer errs, ierr
      integer ndims, nperiods, i, size
      integer comm, source, dest, newcomm
      integer maxdims
      parameter (maxdims=7)
      logical periods(maxdims), outperiods(maxdims)
      integer dims(maxdims), outdims(maxdims)
      integer outcoords(maxdims)

      errs = 0
      call mtest_init( ierr )

!
!     For upto 6 dimensions, test with periodicity in 0 through all
!     dimensions.  The test is computed by both:
!         get info about the created communicator
!         apply cart shift
!     Note that a dimension can have size one, so that these tests
!     can work with small numbers (even 1) of processes
!
      comm = MPI_COMM_WORLD
      call mpi_comm_size( comm, size, ierr )
      do ndims = 1, 6
         do nperiods = 0, ndims
            do i=1,ndims
               periods(i) = .false.
               dims(i)    = 0
            enddo
            do i=1,nperiods
               periods(i) = .true.
            enddo

            call mpi_dims_create( size, ndims, dims, ierr )
            call mpi_cart_create( comm, ndims, dims, periods, .false., &
      &           newcomm, ierr )

            if (newcomm .ne. MPI_COMM_NULL) then
               call mpi_cart_get( newcomm, maxdims, outdims, outperiods, &
      &              outcoords, ierr )
!               print *, 'Coords = '
               do i=1, ndims
!                  print *, i, '(', outcoords(i), ')'
                  if (periods(i) .neqv. outperiods(i)) then
                     errs = errs + 1
                     print *, ' Wrong value for periods ', i
                     print *, ' ndims = ', ndims
                  endif
               enddo

               do i=1, ndims
                  call mpi_cart_shift( newcomm, i-1, 1, source, dest, &
      &                 ierr )
                  if (outcoords(i) .eq. outdims(i)-1) then
                     if (periods(i)) then
                        if (dest .eq. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected rank, got proc_null'
                        endif
                     else
                        if (dest .ne. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected procnull, got ', dest
                        endif
                     endif
                  endif

                  call mpi_cart_shift( newcomm, i-1, -1, source, dest, &
      &                 ierr )
                  if (outcoords(i) .eq. 0) then
                     if (periods(i)) then
                        if (dest .eq. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected rank, got proc_null'
                        endif
                     else
                        if (dest .ne. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected procnull, got ', dest
                        endif
                     endif
                  endif
               enddo
               call mpi_comm_free( newcomm, ierr )
            endif

         enddo
      enddo

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
