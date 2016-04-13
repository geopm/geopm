#include <mpi.h>
#include "geopm.h"

int main(int argc, char **argv)
{
    int ierr = 0;
    int index = 0;
    double sum = 0.0;
    struct geopm_prof_c *prof = NULL;
    uint64_t region_id = 0;

    ierr = MPI_Init(&argc, &argv);
    if (!ierr) {
        ierr = geopm_prof_create("timed_loop", "/geopm_shm_key", MPI_COMM_WORLD, &prof);
    }
    if (!ierr) {
        ierr = geopm_prof_region(NULL, "loop_0", GEOPM_POLICY_HINT_UNKNOWN, &region_id);
    }
    if (!ierr) {
        ierr = geopm_prof_enter(NULL, region_id);
    }
    if (!ierr) {
        for (index = 0; index < 100000; ++index) {
            sum += (double)index;
        }
        ierr = geopm_prof_exit(NULL, region_id);
    }
    if (!ierr) {
        ierr = geopm_prof_print(prof, "timed_loop", 0);
    }
    if (!ierr) {
        ierr = geopm_prof_destroy(prof);
    }

    int tmp_err = MPI_Finalize();

    return ierr ? ierr : tmp_err;
}
