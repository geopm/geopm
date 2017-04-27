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

!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
          use mpi
          integer ierr, errs
          integer size1, size2
          real    a
          real    d(20)
          double precision b
          complex c

          errs = 0
          call mpi_init(ierr)
          call mpi_sizeof( errs, size1, ierr )
          call mpi_type_size( MPI_INTEGER, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "integer size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( a, size1, ierr )
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( b, size1, ierr )
          call mpi_type_size( MPI_DOUBLE_PRECISION, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "double precision size is ", size2, " sizeof claims ", size1
          endif

          call mpi_sizeof( c, size1, ierr )
          call mpi_type_size( MPI_COMPLEX, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "complex size is ", size2, " sizeof claims ", size1
          endif
!
! A previous version of this test called mpi_sizeof with a character variable.
! However, the MPI 2.2 standard, p 494, line 41, defines MPI_SIZEOF only
! for "numeric intrinsic type", so that test was removed.
!
          call mpi_sizeof( d, size1, ierr )
          call mpi_type_size( MPI_REAL, size2, ierr )
          if (size1 .ne. size2) then
             errs = errs + 1
             print *, "real array size is ", size2, " sizeof claims ", size1
          endif

          if (errs .gt. 0) then
             print *, ' Found ', errs, ' errors'
          else
             print *, ' No Errors'
          endif
          call mpi_finalize(ierr)

        end program main
