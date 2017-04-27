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
!  (C) 2008 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! To do: Create a keyval from c, then change the attribute from Fortran,
! then dup.  The C attribute copy function should be passed a pointer to
! the Fortran attribute value (e.g., it should dereference it to check
! its value)
!
      program main
      use mpi
      integer (kind=MPI_ADDRESS_KIND) val
      integer ierr, errs, fcomm2_keyval, ftype2_keyval
      integer ccomm2_keyval, ctype2_keyval, cwin2_keyval
      integer callcount, delcount
      integer (kind=MPI_ADDRESS_KIND) commextra, typeextra
      common /myattr/ callcount, delcount
      external mycopyfn, mydelfn, mytcopyfn, mytdelfn

      callcount = 0
      delcount  = 0

      errs      = 0
      call mpi_init(ierr)
      commextra = 1001
      call mpi_comm_create_keyval( mycopyfn, mydelfn,                     &
     &                             fcomm2_keyval, commextra, ierr )
      typeextra = 2001
      call mpi_type_create_keyval( mytcopyfn, mytdelfn,                   &
                                   ftype2_keyval, typeextra, ierr )
      call chkckeyvals( ccomm2_keyval, ctype2_keyval, cwin2_keyval )

      ! Address-sized ints may be 32, 64, or something else in size;
      ! we can't assume any particular size.  We can use the Fortran 90
      ! intrinsic range to determine the available size and compute
      ! a suitable value.
      val = 5555
      call mpi_comm_set_attr( MPI_COMM_WORLD, fcomm2_keyval, val, ierr )

      call chkcomm2inc( fcomm2_keyval, 5555, errs )

      call mpi_comm_free_keyval( fcomm2_keyval, ierr )
      call mpi_type_free_keyval( ftype2_keyval, ierr )
      
      call mpi_comm_free_keyval( ccomm2_keyval, ierr )
      call mpi_type_free_keyval( ctype2_keyval, ierr )
      call mpi_win_free_keyval( cwin2_keyval, ierr )

      if (errs .eq. 0) then
         print *, ' No Errors'
      else
         print *, ' Found ', errs, ' errors'
      endif

      call mpi_finalize(ierr)
      end
!
      subroutine mycopyfn( oldcomm, keyval, extrastate, valin, valout, &
      &                     flag, ierr )
      use mpi
      integer oldcomm, keyval, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      logical flag
      integer callcount, delcount
      common /myattr/ callcount, delcount
! increment the attribute by 2
      valout = valin + 2
      callcount = callcount + 1
      if (extrastate .eq. 1001) then
         flag = .true.
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         flag = .false.
         ierr = MPI_ERR_OTHER
      endif
      end
!
      subroutine mydelfn( comm, keyval, val, extrastate, ierr )
      use mpi
      integer comm, keyval, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer callcount, delcount
      common /myattr/ callcount, delcount
      delcount = delcount + 1
      if (extrastate .eq. 1001) then
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         ierr = MPI_ERR_OTHER
      endif
      end
!
      subroutine mytcopyfn( oldtype, keyval, extrastate, valin, valout, &
      &                     flag, ierr )
      use mpi
      integer oldtype, keyval, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      logical flag
      integer callcount, delcount
      common /myattr/ callcount, delcount
! increment the attribute by 2
      valout = valin + 2
      callcount = callcount + 1
      if (extrastate .eq. 2001) then
         flag = .true.
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         flag = .false.
         ierr = MPI_ERR_OTHER
      endif
      end
!
      subroutine mytdelfn( dtype, keyval, val, extrastate, ierr )
      use mpi
      integer dtype, keyval, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer callcount, delcount
      common /myattr/ callcount, delcount
      delcount = delcount + 1
      if (extrastate .eq. 2001) then
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         ierr = MPI_ERR_OTHER
      endif
      end
