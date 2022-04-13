!
! Copyright (c) 2015 - 2022, Intel Corporation
! SPDX-License-Identifier: BSD-3-Clause

program timed_loop

use geopm_prof
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
integer(4) :: num_iter
integer(4) :: chunk_size
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
!$omp parallel default(shared) private(thread_idx)
    thread_idx = omp_get_thread_num()
    ierr = geopm_tprof_init(num_iter)
!$omp do schedule(static, chunk_size)
    do index = 1, int(num_iter)
        sum = sum + index
        ierr = geopm_tprof_post()
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
