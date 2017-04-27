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

! This file created from test/mpi/f77/io/atomicityf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer (kind=MPI_OFFSET_KIND) disp

! tests whether atomicity semantics are satisfied for overlapping accesses
! in atomic mode. The probability of detecting errors is higher if you run 
! it on 8 or more processes. 
! This is a version of the test in romio/test/atomicity.c .
      integer BUFSIZE
      parameter (BUFSIZE=10000)
      integer writebuf(BUFSIZE), readbuf(BUFSIZE)
      integer i, mynod, nprocs, len, ierr, errs, toterrs
      character*50 filename
      integer newtype, fh, info, status(MPI_STATUS_SIZE)

      errs = 0

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, mynod, ierr )
      call MPI_Comm_size(MPI_COMM_WORLD, nprocs, ierr )

! Unlike the C version, we fix the filename because of the difficulties
! in accessing the command line from different Fortran environments      
      filename = "testfile.txt"
! test atomicity of contiguous accesses 

! initialize file to all zeros 
      if (mynod .eq. 0) then
         call MPI_File_delete(filename, MPI_INFO_NULL, ierr )
         call MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE +  &
      &        MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr )
         do i=1, BUFSIZE
            writebuf(i) = 0
         enddo
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status, &
      &        ierr) 
         call MPI_File_close(fh, ierr )
      endif
      call MPI_Barrier(MPI_COMM_WORLD, ierr )

      do i=1, BUFSIZE
         writebuf(i) = 10
         readbuf(i)  = 20
      enddo

      call MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE +  &
      &     MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr )

! set atomicity to true 
      call MPI_File_set_atomicity(fh, .true., ierr)
      if (ierr .ne. MPI_SUCCESS) then
         print *, "Atomic mode not supported on this file system."
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
      endif

      call MPI_Barrier(MPI_COMM_WORLD, ierr )
    
! process 0 writes and others concurrently read. In atomic mode, 
! the data read must be either all old values or all new values; nothing
! in between. 

      if (mynod .eq. 0) then
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status, &
      &        ierr) 
      else 
         call MPI_File_read(fh, readbuf, BUFSIZE, MPI_INTEGER, status, &
      &        ierr ) 
         if (ierr .eq. MPI_SUCCESS) then
            if (readbuf(1) .eq. 0) then
!              the rest must also be 0 
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 0) then
                     errs = errs + 1
                     print *, "(contig)Process ", mynod, ": readbuf(", i &
      &                    ,") is ", readbuf(i), ", should be 0"
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else if (readbuf(1) .eq. 10) then
!              the rest must also be 10
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 10) then
                     errs = errs + 1
                     print *, "(contig)Process ", mynod, ": readbuf(", i &
      &                    ,") is ", readbuf(i), ", should be 10"  
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else 
               errs = errs + 1
               print *, "(contig)Process ", mynod, ": readbuf(1) is ",  &
      &              readbuf(1), ", should be either 0 or 10"
            endif
         endif
      endif

      call MPI_File_close( fh, ierr )
        
      call MPI_Barrier( MPI_COMM_WORLD, ierr )


! repeat the same test with a noncontiguous filetype 

      call MPI_Type_vector(BUFSIZE, 1, 2, MPI_INTEGER, newtype, ierr)
      call MPI_Type_commit(newtype, ierr )

      call MPI_Info_create(info, ierr )
! I am setting these info values for testing purposes only. It is
! better to use the default values in practice. */
      call MPI_Info_set(info, "ind_rd_buffer_size", "1209", ierr )
      call MPI_Info_set(info, "ind_wr_buffer_size", "1107", ierr )
    
      if (mynod .eq. 0) then
         call MPI_File_delete(filename, MPI_INFO_NULL, ierr )
         call MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE + &
      &        MPI_MODE_RDWR, info, fh, ierr ) 
        do i=1, BUFSIZE
           writebuf(i) = 0
        enddo
        disp = 0
        call MPI_File_set_view(fh, disp, MPI_INTEGER, newtype, "native" &
      &       ,info, ierr) 
        call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status, &
      &       ierr ) 
        call MPI_File_close( fh, ierr )
      endif
      call MPI_Barrier( MPI_COMM_WORLD, ierr )

      do i=1, BUFSIZE
         writebuf(i) = 10
         readbuf(i)  = 20 
      enddo

      call MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE + &
      &     MPI_MODE_RDWR, info, fh, ierr ) 
      call MPI_File_set_atomicity(fh, .true., ierr)
      disp = 0
      call MPI_File_set_view(fh, disp, MPI_INTEGER, newtype, "native", &
      &     info, ierr ) 
      call MPI_Barrier(MPI_COMM_WORLD, ierr )
    
      if (mynod .eq. 0) then 
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status, &
      &        ierr ) 
      else 
         call MPI_File_read(fh, readbuf, BUFSIZE, MPI_INTEGER, status, &
      &        ierr ) 
         if (ierr .eq. MPI_SUCCESS) then
            if (readbuf(1) .eq. 0) then
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 0) then
                     errs = errs + 1
                     print *, "(noncontig)Process ", mynod, ": readbuf(" &
      &                    , i,") is ", readbuf(i), ", should be 0" 
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else if (readbuf(1) .eq. 10) then
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 10) then
                     errs = errs + 1
                     print *, "(noncontig)Process ", mynod, ": readbuf(" &
      &                    , i,") is ", readbuf(i), ", should be 10"
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else 
               errs = errs + 1
               print *, "(noncontig)Process ", mynod, ": readbuf(1) is " &
      &              ,readbuf(1), ", should be either 0 or 10" 
            endif
         endif
      endif

      call MPI_File_close( fh, ierr )
        
      call MPI_Barrier(MPI_COMM_WORLD, ierr )

      call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, &
      &     MPI_COMM_WORLD, ierr )
      if (mynod .eq. 0) then
         if( toterrs .gt. 0) then
            print *, "Found ", toterrs, " errors"
         else 
            print *, " No Errors"
         endif
      endif

      call MPI_Type_free(newtype, ierr )
      call MPI_Info_free(info, ierr )
      
      call MPI_Finalize(ierr)
      end
