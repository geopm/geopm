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
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This test contributed by Kim McMahon, Cray
!
      program main
      use mpi
      implicit none
      integer ierr, i, j, type, count,errs
      parameter (count = 4)
      integer rank, size, xfersize
      integer status(MPI_STATUS_SIZE)
      integer blocklens(count), displs(count)
      double precision,dimension(:,:),allocatable :: sndbuf, rcvbuf
      logical verbose

      verbose = .false. 
      call mtest_init ( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      if (size .lt. 2) then
         print *, "Must have at least 2 processes"
         call MPI_Abort( MPI_COMM_WORLD, 1, ierr )
      endif

      errs = 0
      allocate(sndbuf(7,100))
      allocate(rcvbuf(7,100))

      do j=1,100
        do i=1,7
           sndbuf(i,j) = (i+j) * 1.0
         enddo
      enddo

      do i=1,count
         blocklens(i) = 7
      enddo

! bug occurs when first two displacements are 0
      displs(1) = 0 
      displs(2) = 0 
      displs(3) = 10
      displs(4) = 10 

      call mpi_type_indexed( count, blocklens, displs*blocklens(1),  &
      &                         MPI_DOUBLE_PRECISION, type, ierr )

      call mpi_type_commit( type, ierr )

! send using this new type

      if (rank .eq. 0) then

          call mpi_send( sndbuf(1,1), 1, type, 1, 0, MPI_COMM_WORLD,ierr )

      else if (rank .eq. 1) then
       
          xfersize=count * blocklens(1)
          call mpi_recv( rcvbuf(1,1), xfersize, MPI_DOUBLE_PRECISION, 0, 0, &
           &   MPI_COMM_WORLD,status, ierr )


! Values that should be sent

        if (verbose) then
!       displacement = 0
            j=1
            do i=1, 7
               print*,'sndbuf(',i,j,') = ',sndbuf(i,j)
            enddo

!       displacement = 10
            j=11
            do i=1,7
               print*,'sndbuf(',i,j,') = ',sndbuf(i,j)
            enddo
            print*,' '

! Values received
            do j=1,count
                do i=1,7
                    print*,'rcvbuf(',i,j,') = ',rcvbuf(i,j)
                enddo
            enddo
        endif

! Error checking
        do j=1,2
           do i=1,7
             if (rcvbuf(i,j) .ne. sndbuf(i,1)) then
                print*,'ERROR in rcvbuf(',i,j,')'
                print*,'Received ', rcvbuf(i,j),' expected ',sndbuf(i,11)
                errs = errs+1
             endif
           enddo
        enddo

        do j=3,4
           do i=1,7
              if (rcvbuf(i,j) .ne. sndbuf(i,11)) then
                print*,'ERROR in rcvbuf(',i,j,')'
                print*,'Received ', rcvbuf(i,j),' expected ',sndbuf(i,11)
                errs = errs+1
              endif
           enddo
        enddo
      endif
!
      call mpi_type_free( type, ierr )
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
