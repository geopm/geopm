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
!  (C) 2013 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!

! Based on a test written by Jim Hoekstra on behalf of Cray, Inc.
! see ticket #884 https://trac.mpich.org/projects/mpich/ticket/884

PROGRAM get_elem_u

  USE mpi 
  IMPLICIT NONE 
  INTEGER    RANK, SIZE, IERR, COMM, errs 
  INTEGER    MAX, I, K, dest
  INTEGER   STATUS(MPI_STATUS_SIZE)

  INTEGER, PARAMETER :: nb=2
  INTEGER :: blklen(nb)=(/1,1/)
  INTEGER :: types(nb)=(/MPI_DOUBLE_PRECISION,MPI_CHAR/)
  INTEGER(kind=MPI_ADDRESS_KIND) :: disp(nb)=(/0,8/)

  INTEGER, PARAMETER :: amax=200
  INTEGER :: type1, type2, extent
  REAL    :: a(amax)

  errs = 0 
  CALL MPI_Init( ierr ) 
  COMM = MPI_COMM_WORLD 
  CALL MPI_Comm_rank(COMM,RANK,IERR) 
  CALL MPI_Comm_size(COMM,SIZE,IERR) 
  dest=size-1

  CALL MPI_Type_create_struct(nb, blklen, disp, types, type1, ierr)
  CALL MPI_Type_commit(type1, ierr)
  CALL MPI_Type_extent(type1, extent, ierr)

  CALL MPI_Type_contiguous(4, Type1, Type2, ierr) 
  CALL MPI_Type_commit(Type2, ierr) 
  CALL MPI_Type_extent(Type2, extent, ierr)

  DO k=1,17

     IF(rank .EQ. 0) THEN 

        !       send k copies of datatype Type1
        CALL MPI_Send(a, k, Type1, dest, 0, comm, ierr) 

     ELSE IF (rank == dest) THEN

        CALL MPI_Recv(a, 200, Type2, 0, 0, comm, status, ierr) 
        CALL MPI_Get_elements(status, Type2, i, ierr)
        IF (i .NE. 2*k) THEN
           errs = errs+1
           PRINT *, "k=",k,"  MPI_Get_elements returns", i, ", but it should be", 2*k
        END IF

     ELSE
        !       thix rank does not particupate
     END IF
  enddo

  CALL MPI_Type_free(type1, ierr)
  CALL MPI_Type_free(type2, ierr)

  CALL MPI_Finalize( ierr )

  IF(rank .EQ. 0 .AND. errs .EQ. 0) THEN
     PRINT *, " No Errors"
  END IF

END PROGRAM get_elem_u
