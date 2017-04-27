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

program get_elem_d

  use mpi
  implicit none

  integer, parameter :: verbose=0
  integer, parameter :: cmax=100,dmax=100,imax=60
  integer, parameter :: nb=2
  integer :: comm,rank,size,dest,ierror,errs=0
  integer :: status(MPI_STATUS_SIZE)
  integer :: i,ii,count,ka,j,jj,k,kj
  integer :: blklen(nb)=(/2,2/)
  integer :: types(nb)=(/MPI_DOUBLE_PRECISION,MPI_INTEGER/)
  integer(kind=MPI_ADDRESS_KIND) :: disp(nb)
  integer :: newtype,ntlen,ians(20),ians0(0:3),ians1(24),ians2(20)
  double precision :: dbuff(dmax), a
  integer :: ibuff(imax)
  character :: cbuff(cmax)='X'

  call MPI_Init(ierror)
  comm=MPI_COMM_WORLD
  call MPI_Comm_size(comm, size, ierror)
  dest=size-1
  call MPI_Comm_rank(comm, rank, ierror)
  call MPI_Sizeof (j, kj, ierror)
  call MPI_Sizeof (a, ka, ierror)
  ntlen=2*ka+2*kj
  disp=(/0,2*ka/)

  !  calculate answers for expected i values for Get_elements with derived type
  ians0(0)=ka
  ians0(1)=2*ka
  ians0(2)=2*ka+kj
  ians0(3)=2*ka+2*kj
  ii=0
  do i=1,24 ! answers for the test sending 1~24 bytes
     if (i .eq. ians0(ii)) ii=ii+1
     ians1(i)=ii
  enddo
  if (rank == 0 .and. verbose > 0) print *, (ians1(k),k=1,24)
  jj=0
  do j=1,17,4 ! 4 means newtype has 4 primitives
     ians(j)=jj+ka/kj
     ians(j+1)=jj+2*(ka/kj)
     ians(j+2)=jj+2*(ka/kj)+1
     ians(j+3)=jj+2*(ka/kj)+2
     if (rank == 0 .and. verbose > 0) print *, (ians(k),k=j,j+3)
     jj=jj+ntlen/kj
  enddo
  ! To have k elements, need to receive ians(k) integers

  ii=0
  do i=1,20 ! answers for the test sending 1~20 integers
     if (i .eq. ians(ii+1)) ii=ii+1
     ians2(i)=ii
  enddo
  if (rank == 0 .and. verbose > 0) print *, (ians2(k),k=1,20)

  if (verbose > 0) print *, MPI_UNDEFINED

  call MPI_Type_create_struct(nb, blklen, disp, types, newtype, ierror)
  call MPI_Type_commit(newtype, ierror)

  do i=1,24 ! sending 1~24 bytes
     if (rank == 0) then
        call MPI_Send(cbuff, i, MPI_BYTE, dest, 100, comm, ierror)

     else if (rank == dest) then

        !     first receive
        call MPI_Recv(dbuff, dmax, newtype, 0, 100, comm, status, ierror)
        !       check on MPI_Get_elements
        call MPI_Get_elements(status, newtype, count, ierror)
        if (count .ne. ians1(i)) then
           errs=errs+1
           write (*,fmt="(i2,'  R1 Get_elements  count=',i3,&
                &'  but should be ',i3)") i,count,ians1(i)
        endif

     else
        !     other ranks do not participate
     endif
  enddo

  do i=1,20 ! sending 1~20 integers
     if (rank == 0) then
        call MPI_Send(ibuff, i, MPI_INTEGER, dest, 100, comm, ierror)

     else if (rank == dest) then

        !     second receive
        call MPI_Recv(dbuff, dmax, newtype, 0, 100, comm, status, ierror)
        !       check on MPI_Get_elements
        call MPI_Get_elements(status, newtype, count, ierror)
        if (count .ne. ians2(i)) then
           errs=errs+1
           write (*,fmt="(i2,'  R2 Get_elements  count=',i3,&
                &'  but should be ',i3)") i,count,ians2(i)
        endif
     else
        !     other ranks do not participate
     endif
  enddo

  if (rank .eq. dest) then
     if (errs .eq. 0) then
        write (*,*) " No Errors"
     else
        print *, 'errs=',errs
     endif
  endif

  call MPI_Type_free(newtype, ierror)
  call MPI_Finalize(ierror)

end program get_elem_d
