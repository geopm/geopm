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
! Thanks to
! William R. Magro
! for this test
!
! It has been modifiedly slightly to work with the automated MPI
! tests.
!  WDG.
!
! It was further modified to use MPI_Get_address instead of MPI_Address
! for MPICH, and to fit in the MPICH test harness - WDG
!
      program bustit
      use mpi
      implicit none

      integer comm
      integer newtype
      integer me
      integer position
      integer type(5)
      integer length(5)
      integer (kind=MPI_ADDRESS_KIND) disp(5)
      integer bufsize
      integer errs, toterrs
      parameter (bufsize=100)
      character buf(bufsize)
      character name*(10)
      integer status(MPI_STATUS_SIZE)
      integer i, size
      double precision x
      integer src, dest
      integer ierr

      errs = 0
!     Enroll in MPI
      call mpi_init(ierr)

!     get my rank
      call mpi_comm_rank(MPI_COMM_WORLD, me, ierr)
      call mpi_comm_size(MPI_COMM_WORLD, size, ierr )
      if (size .lt. 2) then
         print *, "Must have at least 2 processes"
         call MPI_Abort( MPI_COMM_WORLD, 1, ierr )
      endif

      comm = MPI_COMM_WORLD
      src = 0
      dest = 1

      if(me.eq.src) then
          i=5
          x=5.1234d0
          name="Hello"

          type(1)=MPI_CHARACTER
          length(1)=5
          call mpi_get_address(name,disp(1),ierr)

          type(2)=MPI_DOUBLE_PRECISION
          length(2)=1
          call mpi_get_address(x,disp(2),ierr)

          call mpi_type_create_struct(2,length,disp,type,newtype,ierr)
          call mpi_type_commit(newtype,ierr)
          call mpi_barrier( MPI_COMM_WORLD, ierr )
          call mpi_send(MPI_BOTTOM,1,newtype,dest,1,comm,ierr)
          call mpi_type_free(newtype,ierr)
!         write(*,*) "Sent ",name(1:5),x
      else
!         Everyone calls barrier incase size > 2
          call mpi_barrier( MPI_COMM_WORLD, ierr )
          if (me.eq.dest) then
             position=0

             name = " "
             x    = 0.0d0
             call mpi_recv(buf,bufsize,MPI_PACKED, src,                    &
     &            1, comm, status, ierr)

             call mpi_unpack(buf,bufsize,position,                         &
     &            name,5,MPI_CHARACTER, comm,ierr)
             call mpi_unpack(buf,bufsize,position,                         &
     &            x,1,MPI_DOUBLE_PRECISION, comm,ierr)
!            Check the return values (/= is not-equal in F90)
             if (name /= "Hello") then
                errs = errs + 1
                print *, "Received ", name, " but expected Hello"
             endif
             if (abs(x-5.1234) .gt. 1.0e-6) then
                errs = errs + 1
                print *, "Received ", x, " but expected 5.1234"
             endif
          endif
      endif
!
!     Sum up errs and report the result
      call mpi_reduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, 0,         &
     &                 MPI_COMM_WORLD, ierr )
      if (me .eq. 0) then
         if (toterrs .eq. 0) then
            print *, " No Errors"
         else
            print *, " Found ", toterrs, " errors"
         endif
      endif

      call mpi_finalize(ierr)

      end
