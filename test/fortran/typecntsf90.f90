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

! This file created from test/mpi/f77/datatype/typecntsf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi
       integer errs, ierr
       integer ntype1, ntype2
!
! This is a very simple test that just tests that the contents/envelope
! routines can be called.  This should be upgraded to test the new 
! MPI-2 datatype routines (which use address-sized integers)
!

       errs = 0
       call mtest_init( ierr )

       call explore( MPI_INTEGER, MPI_COMBINER_NAMED, errs )
       call explore( MPI_BYTE, MPI_COMBINER_NAMED, errs )
       call mpi_type_vector( 10, 1, 30, MPI_DOUBLE_PRECISION, ntype1,  &
      &                       ierr )
       call explore( ntype1, MPI_COMBINER_VECTOR, errs )
       call mpi_type_dup( ntype1, ntype2, ierr )
       call explore( ntype2, MPI_COMBINER_DUP, errs )
       call mpi_type_free( ntype2, ierr )
       call mpi_type_free( ntype1, ierr )
       
!
       call mtest_finalize( errs )
       call mpi_finalize( ierr )
       end
!
       subroutine explore( dtype, mycomb, errs )
       use mpi
       integer dtype, mycomb, errs
       integer ierr
       integer nints, nadds, ntype, combiner
       integer max_nints, max_dtypes, max_asizev
       parameter (max_nints = 10, max_dtypes = 10, max_asizev=10)
       integer intv(max_nints), dtypesv(max_dtypes)
       integer (kind=MPI_ADDRESS_KIND) aint, aintv(max_asizev)

!
       call mpi_type_get_envelope( dtype, nints, nadds, ntype, &
      &                             combiner, ierr )
!
       if (combiner .ne. MPI_COMBINER_NAMED) then
          call mpi_type_get_contents( dtype,  &
      &         max_nints, max_asizev, max_dtypes, &
      &         intv, aintv, dtypesv, ierr )
!
!              dtypesv of constructed types must be free'd now
!
          if (combiner .eq. MPI_COMBINER_DUP) then
             call mpi_type_free( dtypesv(1), ierr )
          endif
       endif
       if (combiner .ne. mycomb) then
          errs = errs + 1
          print *, ' Expected combiner ', mycomb, ' but got ', &
      &             combiner
       endif
!
! List all combiner types to check that they are defined in mpif.h
       if (combiner .eq. MPI_COMBINER_NAMED) then
       else if (combiner .eq. MPI_COMBINER_DUP) then
       else if (combiner .eq. MPI_COMBINER_CONTIGUOUS) then
       else if (combiner .eq. MPI_COMBINER_VECTOR) then
       else if (combiner .eq. MPI_COMBINER_HVECTOR_INTEGER) then
       else if (combiner .eq. MPI_COMBINER_HVECTOR) then
       else if (combiner .eq. MPI_COMBINER_INDEXED) then
       else if (combiner .eq. MPI_COMBINER_HINDEXED_INTEGER) then
       else if (combiner .eq. MPI_COMBINER_HINDEXED) then
       else if (combiner .eq. MPI_COMBINER_INDEXED_BLOCK) then
       else if (combiner .eq. MPI_COMBINER_STRUCT_INTEGER) then
       else if (combiner .eq. MPI_COMBINER_STRUCT) then
       else if (combiner .eq. MPI_COMBINER_SUBARRAY) then
       else if (combiner .eq. MPI_COMBINER_DARRAY) then
       else if (combiner .eq. MPI_COMBINER_F90_REAL) then
       else if (combiner .eq. MPI_COMBINER_F90_COMPLEX) then
       else if (combiner .eq. MPI_COMBINER_F90_INTEGER) then
       else if (combiner .eq. MPI_COMBINER_RESIZED) then
       else
          errs = errs + 1
          print *, ' Unknown combiner ', combiner
       endif
       
       return
       end
