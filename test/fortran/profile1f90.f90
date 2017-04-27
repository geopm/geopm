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

! This file created from test/mpi/f77/profile/profile1f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2013 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi
       integer ierr
       integer smsg(3), rmsg(3), toterrs, wsize, wrank
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount

       toterrs = 0
       call mpi_init( ierr )
       call init_counts()
       call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
       call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )

       if (wrank .eq. 0) then
           smsg(1) = 3
           call mpi_send( smsg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, ierr )
       else if (wrank .eq. 1) then
          call mpi_recv( rmsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &
      &         MPI_STATUS_IGNORE, ierr ) 
          if (rmsg(1) .ne. 3) then
             toterrs = toterrs + 1
             print *, "Unexpected received value ", rmsg(1)
          endif
       endif
!
!     check that we used the profiling versions of the routines
       toterrs = 0
       if (wrank .eq. 0) then
          if (calls.ne.1) then
             toterrs = toterrs + 1
             print *, "Sender calls is ", calls
          endif
          if (amount.ne.1) then
             toterrs = toterrs + 1
             print *, "Sender amount is ", amount
          endif
       else if (wrank .eq. 1) then
          if (rcalls.ne.1) then
             toterrs = toterrs + 1
             print *, "Receiver calls is ", rcalls
          endif
          if (ramount.ne.1) then
             toterrs = toterrs + 1
             print *, "Receiver amount is ", ramount
          endif
       endif

       call mpi_allreduce( MPI_IN_PLACE, toterrs, 1, MPI_INT, MPI_SUM, &
      &      MPI_COMM_WORLD, ierr )
       if (wrank .eq. 0) then
          if (toterrs .eq. 0) then
             print *, " No Errors"
          else
             print *, " Found ", toterrs, " errors"
          endif
       endif
!
       call mpi_finalize( ierr )
       end
!
       subroutine mpi_send( smsg, count, dtype, dest, tag, comm, ierr )
       use mpi
       integer count, dtype, dest, tag, comm, ierr
       integer smsg(count)
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
!
       calls = calls + 1
       amount = amount + count
       call pmpi_send( smsg, count, dtype, dest, tag, comm, ierr )
       return
       end
!
      subroutine mpi_recv( rmsg, count, dtype, src, tag, comm, status, &
      &     ierr ) 
       use mpi
       integer count, dtype, src, tag, comm, status(MPI_STATUS_SIZE), &
      &      ierr 
       integer rmsg(count)
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       rcalls = rcalls + 1
       ramount = ramount + 1
       call pmpi_recv( rmsg, count, dtype, src, tag, comm, status, ierr &
      &      ) 
       return
       end
!
       subroutine init_counts()
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       calls = 0
       amount = 0
       rcalls = 0
       ramount = 0
       end
!
       subroutine mpi_pcontrol( ierr )
       integer ierr
       return
       end
