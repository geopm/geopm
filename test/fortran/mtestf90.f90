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

! This file created from test/mpi/f77/util/mtestf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        subroutine MTest_Init( ierr )
!       Place the include first so that we can automatically create a
!       Fortran 90 version that uses the mpi module instead.  If
!       the module is in a different place, the compiler can complain
!       about out-of-order statements
        use mpi
        integer ierr
        logical flag
        logical dbgflag
        integer wrank
        common /mtest/ dbgflag, wrank

        call MPI_Initialized( flag, ierr )
        if (.not. flag) then
           call MPI_Init( ierr )
        endif

        dbgflag = .false.
        call MPI_Comm_rank( MPI_COMM_WORLD, wrank, ierr )
        end
!
        subroutine MTest_Finalize( errs )
        use mpi
        integer errs
        integer rank, toterrs, ierr

        call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )

        call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM,  &
      &        MPI_COMM_WORLD, ierr )

        if (rank .eq. 0) then
           if (toterrs .gt. 0) then
                print *, " Found ", toterrs, " errors"
           else
                print *, " No Errors"
           endif
        endif
        end
!
! A simple get intracomm for now
        logical function MTestGetIntracomm( comm, min_size, qsmaller )
        use mpi
        integer ierr
        integer comm, min_size, size, rank
        logical qsmaller
        integer myindex
        save myindex
        data myindex /0/

        comm = MPI_COMM_NULL
        if (myindex .eq. 0) then
           comm = MPI_COMM_WORLD
        else if (myindex .eq. 1) then
           call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
        else if (myindex .eq. 2) then
           call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
           call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
           call mpi_comm_split( MPI_COMM_WORLD, 0, size - rank, comm,  &
      &                                 ierr )
        else
           if (min_size .eq. 1 .and. myindex .eq. 3) then
              comm = MPI_COMM_SELF
           endif
        endif
        myindex = mod( myindex, 4 ) + 1
        MTestGetIntracomm = comm .ne. MPI_COMM_NULL
        end
!
        subroutine MTestFreeComm( comm )
        use mpi
        integer comm, ierr
        if (comm .ne. MPI_COMM_WORLD .and. &
      &      comm .ne. MPI_COMM_SELF  .and. &
      &      comm .ne. MPI_COMM_NULL) then
           call mpi_comm_free( comm, ierr )
        endif
        end
!
        subroutine MTestPrintError( errcode )
        use mpi
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, "Error class ", errclass, "(", string(1:slen), ")"
        end
!
        subroutine MTestPrintErrorMsg( msg, errcode )
        use mpi
        character*(*) msg
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, msg, ": Error class ", errclass, " &
      &       (", string(1:slen), ")"
        end

        subroutine MTestSpawnPossible( can_spawn, errs )
        use mpi
        integer can_spawn
        integer errs
        integer(kind=MPI_ADDRESS_KIND) val
        integer ierror
        logical flag
        integer comm_size

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, val, &
      &                          flag, ierror )
        if ( ierror .ne. MPI_SUCCESS ) then
!       MPI_UNIVERSE_SIZE keyval missing from MPI_COMM_WORLD attributes
            can_spawn = -1
            errs = errs + 1
        else
            if ( flag ) then
                comm_size = -1

                call mpi_comm_size( MPI_COMM_WORLD, comm_size, ierror )
                if ( ierror .ne. MPI_SUCCESS ) then
!       MPI_COMM_SIZE failed for MPI_COMM_WORLD
                    can_spawn = -1
                    errs = errs + 1
                    return
                endif

                if ( val .le. comm_size ) then
!       no additional processes can be spawned
                    can_spawn = 0
                else
                    can_spawn = 1
                endif
            else
!       No attribute associated with key MPI_UNIVERSE_SIZE of MPI_COMM_WORLD
                can_spawn = -1
            endif
        endif
        end
