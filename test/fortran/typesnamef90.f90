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

! This file created from test/mpi/f77/datatype/typesnamef.f with f77tof90
! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi
       character*(MPI_MAX_OBJECT_NAME) cname
       integer rlen, ln
       integer ntype1, ntype2, errs, ierr

       errs = 0
       
       call MTest_Init( ierr )

       call mpi_type_vector( 10, 1, 100, MPI_INTEGER, ntype1, ierr )
       rlen = -1
       cname = 'XXXXXX'
       call mpi_type_get_name( ntype1, cname, rlen, ierr )
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' Expected length 0, got ', rlen
       endif
       rlen = 0
       do ln=MPI_MAX_OBJECT_NAME,1,-1
          if (cname(ln:ln) .ne. ' ') then
             rlen = ln
             goto 100
          endif
       enddo
 100   continue
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, 'Datatype name is not all blank'
       endif
!
! now add a name, then dup
       call mpi_type_set_name( ntype1, 'a vector type', ierr )
       call mpi_type_dup( ntype1, ntype2, ierr )
       rlen = -1
       cname = 'XXXXXX'
       call mpi_type_get_name( ntype2, cname, rlen, ierr )
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' (type2) Expected length 0, got ', rlen
       endif
       rlen = 0
       do ln=MPI_MAX_OBJECT_NAME,1,-1
          if (cname(ln:ln) .ne. ' ') then
             rlen = ln
             goto 110
          endif
       enddo
 110   continue
       if (rlen .ne. 0) then
          errs = errs + 1
          print *, ' (type2) Datatype name is not all blank'
       endif
       
       call mpi_type_free( ntype1, ierr )
       call mpi_type_free( ntype2, ierr )
       
       call MTest_Finalize( errs )
       call MPI_Finalize( ierr )

       end
