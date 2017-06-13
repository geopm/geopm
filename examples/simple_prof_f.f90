!
! Copyright (c) 2015, 2016, 2017, Intel Corporation
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions
! are met:
!
!     * Redistributions of source code must retain the above copyright
!       notice, this list of conditions and the following disclaimer.
!
!     * Redistributions in binary form must reproduce the above copyright
!       notice, this list of conditions and the following disclaimer in
!       the documentation and/or other materials provided with the
!       distribution.
!
!     * Neither the name of Intel Corporation nor the names of its
!       contributors may be used to endorse or promote products derived
!       from this software without specific prior written permission.
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
! (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
! OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
!

program timed_loop

use geopm
use, intrinsic :: ISO_C_BINDING

implicit none

include 'mpif.h'

integer, external :: omp_get_thread_num, omp_get_num_threads
integer :: ierr
integer :: index
real(kind=c_double) :: sum
integer(8) :: region_id
integer :: rank
integer(4) :: num_thread
integer(8) :: num_iter
integer(8) :: chunk_size
integer(4) :: thread_idx

num_iter = 100000000
chunk_size = 1000

call MPI_Init(ierr)
ierr = geopm_prof_region(c_char_'loop_0'//c_null_char, GEOPM_REGION_HINT_UNKNOWN, region_id)
ierr = geopm_prof_enter(region_id)
sum = 0
!$omp parallel
    num_thread = omp_get_num_threads()
!$omp end parallel
    ierr = geopm_tprof_reset(num_thread, num_iter, chunk_size, c_null_ptr)
!$omp parallel default(shared) private(thread_idx)
    thread_idx = omp_get_thread_num()
!$omp do schedule(static, chunk_size)
    do index = 1, int(num_iter)
        sum = sum + index
        ierr = geopm_tprof_increment()
    end do
!$omp end do
!$omp end parallel

call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
if (rank == 0) then
    print *,'sum=', sum
end if
ierr = geopm_prof_exit(region_id)
call MPI_Finalize(ierr)

end program timed_loop
