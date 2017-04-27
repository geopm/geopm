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

! This file created from test/mpi/f77/spawn/spawnf.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi
        integer errs, err
        integer rank, size, rsize, i
        integer np
        integer errcodes(2)
        integer parentcomm, intercomm
        integer status(MPI_STATUS_SIZE)
        integer ierr
        integer can_spawn

        errs = 0
        np   = 2

        call MTest_Init( ierr )

        call MTestSpawnPossible( can_spawn, errs )
        if ( can_spawn .eq. 0 ) then
            call MTest_Finalize( errs )
            goto 300
        endif

        call MPI_Comm_get_parent( parentcomm, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
!          Create 2 more processes 
           call MPI_Comm_spawn( "./spawnf90", MPI_ARGV_NULL, np, &
      &          MPI_INFO_NULL, 0, MPI_COMM_WORLD, intercomm, errcodes &
      &          ,ierr ) 
        else
           intercomm = parentcomm
        endif

!   We now have a valid intercomm 

        call MPI_Comm_remote_size( intercomm, rsize, ierr )
        call MPI_Comm_size( intercomm, size, ierr )
        call MPI_Comm_rank( intercomm, rank, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
!           Master
           if (rsize .ne. np) then
              errs = errs + 1
              print *, "Did not create ", np, " processes (got ", rsize, &
      &             ")" 
           endif
           if (rank .eq. 0) then
              do i=0,rsize-1
                 call MPI_Send( i, 1, MPI_INTEGER, i, 0, intercomm, ierr &
      &                ) 
              enddo
!       We could use intercomm reduce to get the errors from the 
!       children, but we'll use a simpler loop to make sure that
!       we get valid data 
              do i=0, rsize-1
                 call MPI_Recv( err, 1, MPI_INTEGER, i, 1, intercomm, &
      &                MPI_STATUS_IGNORE,  ierr ) 
                errs = errs + err
             enddo
          endif
        else 
!             Child 
           if (size .ne. np) then
              errs = errs + 1
              print *, "(Child) Did not create ", np, " processes (got " &
      &             ,size, ")" 
           endif

           call MPI_Recv( i, 1, MPI_INTEGER, 0, 0, intercomm, status, &
      &          ierr ) 

        if (i .ne. rank) then
            errs = errs + 1
            print *, "Unexpected rank on child ", rank, "(",i,")"
         endif

!       Send the errs back to the master process 
         call MPI_Ssend( errs, 1, MPI_INTEGER, 0, 1, intercomm, ierr )
        endif

!       It isn't necessary to free the intercomm, but it should not hurt
        call MPI_Comm_free( intercomm, ierr )

!       Note that the MTest_Finalize get errs only over COMM_WORLD 
!       Note also that both the parent and child will generate "No
!       Errors" if both call MTest_Finalize 
        if (parentcomm .eq. MPI_COMM_NULL) then
           call MTest_Finalize( errs )
        endif

 300    continue
        call MPI_Finalize( ierr )
        end
