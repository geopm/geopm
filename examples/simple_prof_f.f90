program timed_loop

use geopm
use, intrinsic :: ISO_C_BINDING

implicit none

include 'mpif.h'

integer ierr
integer index
real(kind=c_double) sum
type(c_ptr) :: geopm_prof
integer(8) :: region_id
integer rank

call MPI_Init(ierr)
ierr = geopm_prof_create(c_char_'timed_loop'//c_null_char, c_char_''//c_null_char, MPI_COMM_WORLD, geopm_prof)
ierr = geopm_prof_region(c_null_ptr, c_char_'loop_0'//c_null_char, GEOPM_POLICY_HINT_UNKNOWN, region_id)
ierr = geopm_prof_enter(c_null_ptr, region_id)
sum = 0
do index = 1, 100000000
    sum = sum + index
end do
call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
if (rank == 0) then
    print *,'sum=', sum
end if
ierr = geopm_prof_exit(c_null_ptr, region_id)
ierr = geopm_prof_print(geopm_prof, c_char_'timed_loop'//c_null_char, 0)
ierr = geopm_prof_destroy(geopm_prof)
call MPI_Finalize(ierr)

end program timed_loop
