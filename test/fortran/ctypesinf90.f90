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

! This file created from test/mpi/f77/ext/ctypesinf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2008 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer ierr
      integer errs, wrank
      integer f2ctype
!
      call mtest_init( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
!
      errs = 0
!
      errs = errs + f2ctype( MPI_CHAR, 0 )
      errs = errs + f2ctype( MPI_SIGNED_CHAR, 1 )
      errs = errs + f2ctype( MPI_UNSIGNED_CHAR, 2 )
      errs = errs + f2ctype( MPI_WCHAR, 3 )
      errs = errs + f2ctype( MPI_SHORT, 4 )
      errs = errs + f2ctype( MPI_UNSIGNED_SHORT, 5 )
      errs = errs + f2ctype( MPI_INT, 6 )
      errs = errs + f2ctype( MPI_UNSIGNED, 7 )
      errs = errs + f2ctype( MPI_LONG, 8 )
      errs = errs + f2ctype( MPI_UNSIGNED_LONG, 9 )
      errs = errs + f2ctype( MPI_FLOAT, 10 )
      errs = errs + f2ctype( MPI_DOUBLE, 11 )
      errs = errs + f2ctype( MPI_FLOAT_INT, 12 )
      errs = errs + f2ctype( MPI_DOUBLE_INT, 13 )
      errs = errs + f2ctype( MPI_LONG_INT, 14 )
      errs = errs + f2ctype( MPI_SHORT_INT, 15 )
      errs = errs + f2ctype( MPI_2INT, 16 )
      if (MPI_LONG_DOUBLE .ne. MPI_DATATYPE_NULL) then
          errs = errs + f2ctype( MPI_LONG_DOUBLE, 17 )
          errs = errs + f2ctype( MPI_LONG_DOUBLE_INT, 21 )
      endif
      if (MPI_LONG_LONG .ne. MPI_DATATYPE_NULL) then
          errs = errs + f2ctype( MPI_LONG_LONG_INT, 18 )
          errs = errs + f2ctype( MPI_LONG_LONG, 19 )
          errs = errs + f2ctype( MPI_UNSIGNED_LONG_LONG, 20 )
      endif
!
! Summarize the errors
!
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end 
