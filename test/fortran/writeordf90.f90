! -*- Mode: Fortran90; -*-
! 
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! DO NOT EDIT THIS FILE.  CREATED AUTOMATICALLY WITH TESTMERGE.
!
        program main
        use mpi
        integer maxfparm
        parameter (maxfparm=5)
        integer max_buffer
        parameter (max_buffer=65536)
        integer maxftype
        parameter (maxftype=3)
!
        integer comm, fh, ftype, itmp, fparm, n, b, i, k, r, s
        integer intsize
        integer errs, toterrs, err, ierr
        integer wrank, wsize
        integer fparms(2,maxfparm)
        character *(100) filename
        integer status(MPI_STATUS_SIZE)
        integer buf(MAX_BUFFER), ans
        integer (kind=MPI_OFFSET_KIND)offset
        data fparms/1,4000,  4000,8,  4096,8,  64000,8,  65536,8 /
        errs = 0
        call MTest_Init( ierr )
        call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
        call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
        call mpi_type_size( MPI_INTEGER, intsize, ierr )
        do ftype = 1, maxftype
            filename = "iotest.txt"
            if (ftype .eq. 1) then
                comm = MPI_COMM_WORLD
            elseif (ftype .eq. 2) then
                call mpi_comm_split( MPI_COMM_WORLD, 0, wsize - wrank,    &
     & comm, ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
            else 
                itmp = 1
                if (wrank .eq. 0) itmp = 0
                call mpi_comm_split( MPI_COMM_WORLD, itmp, wrank, comm,   &
     & ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
                if (wrank .eq. 0) then
                    goto 10
                endif
            endif
            call mpi_comm_size( comm, s, ierr )
            call mpi_comm_rank( comm, r, ierr )
            do fparm=1, maxfparm
                n = fparms(1,fparm)
                b = fparms(2,fparm)
! Try writing the file, then check it
                call mpi_file_open( comm, filename, MPI_MODE_RDWR +       &
     & MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
                if (ierr .ne. MPI_SUCCESS) then
                    goto 111
                endif
                do k=1, b
                   do i=1, n
                       buf(i) = r*n + (k-1)*n*s + i-1
                   enddo
                   call mpi_file_write_ordered( fh, buf, n, MPI_INTEGER   &
     &, status, ierr )
                   if (ierr .ne. MPI_SUCCESS) then
                       errs = errs + 1
                       if (errs .le. 10) then
                           call MTestPrintError( ierr )
                       endif
                   endif
                enddo
                call mpi_file_close( fh, ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
! Now, open the same file for reading
                call mpi_file_open( comm, filename, MPI_MODE_RDWR +       &
     & MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
                if (ierr .ne. MPI_SUCCESS) then
                    goto 111
                endif
                do k=1, b
                    do i=1, n
                        buf(i) = - (r*n + (k-1)*n*s + i)
                    enddo
                    call mpi_file_read_ordered( fh, buf, n, MPI_INTEGER   &
     &, status, ierr )
                    if (ierr .ne. MPI_SUCCESS) then
                        errs = errs + 1
                        if (errs .le. 10) then
                            call MTestPrintError( ierr )
                        endif
                    endif
                    do i=1, n
                        ans = r*n + (k-1)*n*s + i-1
                        if (buf(i) .ne. ans) then
                            errs = errs + 1
                            if (errs .le. 10) then
                                print *, r, k, ' buf(',i,') = ', buf(i)   &
     &, ' expected ', ans
                            endif
                        endif
                    enddo
                enddo
                call mpi_file_close( fh, ierr )
                if (ierr .ne. MPI_SUCCESS) then
                    errs = errs + 1
                    if (errs .le. 10) then
                        call MTestPrintError( ierr )
                    endif
                endif
                call mpi_barrier( comm, ierr )
                call mpi_comm_rank( comm, r, ierr )
                if (r .eq. 0) then
                    call mpi_file_delete( filename, MPI_INFO_NULL, ierr   &
     & )
                    if (ierr .ne. MPI_SUCCESS) then
                        errs = errs + 1
                        if (errs .le. 10) then
                            call MTestPrintError( ierr )
                        endif
                    endif
                endif
                call mpi_barrier( comm, ierr )
            enddo
10      continue
!
! Jump to 111 on a failure to open
111	continue
        if (comm .ne. MPI_COMM_WORLD) then
            call mpi_comm_free( comm, ierr )
        endif
        enddo
        call MTest_Finalize( errs )
        call mpi_finalize( ierr )
        end
