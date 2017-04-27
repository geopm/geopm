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
! This is a simple test to check that both the MPI and PMPI versions of the
! timers are available, and that they return double precision values.
! If this code links, there are no problems.
!
          use mpi
          implicit none
          integer err
          double precision time1

          call mpi_init(err)

          time1 = mpi_wtime()
          time1 = time1 + mpi_wtick()
          time1 = time1 + pmpi_wtime()
          time1 = time1 + pmpi_wtick()
! Add a test on time1 to ensure that the compiler does not remove the calls
! (The compiler should call them anyway because they aren't pure, but
! including these operations ensures that a buggy compiler doesn't
! pass this test by mistake).
          if (time1 .lt. 0.0d0) then
             print *, ' Negative time result'
          else
                print *, ' No Errors'
          endif

          call mpi_finalize(err)
        end
