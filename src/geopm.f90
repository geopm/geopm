!
! Copyright (c) 2015, 2016, Intel Corporation
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

module geopm
    use, intrinsic :: ISO_C_BINDING
    implicit none

    enum, bind(C)
        enumerator :: GEOPM_POLICY_HINT_UNKNOWN = 0
        enumerator :: GEOPM_POLICY_HINT_COMPUTE = 1
        enumerator :: GEOPM_POLICY_HINT_MEMORY = 2
        enumerator :: GEOPM_POLICY_HINT_NETWORK = 3
    end enum

    interface
        !!!!!!!!!!!!!!!!!!!!!!!!!!
        !! OBJECT INSTANTIATION !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!

        integer(kind=c_int) function geopm_ctl_create(policy, sample_key, comm, ctl) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: policy
            character(kind=c_char), intent(in) :: sample_key(*)
            integer(kind=c_int), intent(in) :: comm
            type(c_ptr) :: ctl(*)
        end function geopm_ctl_create

        integer(kind=c_int) function geopm_ctl_destroy(ctl) bind(C)
            import
            implicit none
            type(c_ptr) :: ctl
        end function geopm_ctl_destroy

        !!!!!!!!!!!!!!!!!!!!!!
        !! POWER MANAGEMENT !!
        !!!!!!!!!!!!!!!!!!!!!!

        integer(kind=c_int) function geopm_ctl_step(ctl) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: ctl
        end function geopm_ctl_step

        integer(kind=c_int) function geopm_ctl_run(ctl) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: ctl
        end function geopm_ctl_run

!        integer(kind=c_int) function geopm_ctl_pthread(ctl, attr, thread) bind(C)
!            import
!            implicit none
!            type(c_ptr), intent(in) :: ctl
!            type(pthread_attr), intent(in) :: attr(*)
!            type(pthread_t), intent(in) :: thread(*)
!        end function geopm_ctl_pthread


        integer(kind=c_int) function geopm_ctl_spawn(ctl) bind(C)
            import
            implicit none
            type(c_ptr) :: ctl
        end function geopm_ctl_spawn

        !!!!!!!!!!!!!!!!!!!!!!!!!!!
        !! APPLICATION PROFILING !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!!

        integer(kind=c_int) function geopm_prof_create(name, shm_key, comm, prof) bind(C)
            import
            implicit none
            character(kind=c_char), intent(in) :: name(*)
            character(kind=c_char), intent(in) :: shm_key(*)
            integer(kind=c_int), intent(in) :: comm
            type(c_ptr) :: prof(*)
        end function geopm_prof_create

        integer(kind=c_int) function geopm_prof_destroy(prof) bind(C)
            import
            implicit none
            type(c_ptr) :: prof
        end function geopm_prof_destroy

        integer(kind=c_int) function geopm_prof_default(prof) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
        end function geopm_prof_default

        integer(kind=c_int) function geopm_prof_region(prof, region_name, policy_hint, region_id) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            character(kind=c_char), intent(in) :: region_name(*)
            integer(kind=c_int), intent(in) :: policy_hint
            type(c_ptr) :: region_id
        end function geopm_prof_region

        integer(kind=c_int) function geopm_prof_enter(prof, region_id) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            integer(kind=c_int64_t), intent(in) :: region_id
        end function geopm_prof_enter

        integer(kind=c_int) function geopm_prof_exit(prof, region_id) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            integer(kind=c_int64_t), intent(in) :: region_id
        end function geopm_prof_exit

        integer(kind=c_int) function geopm_prof_progress(prof, region_id, fraction) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            integer(kind=c_int64_t), intent(in) :: region_id
            real(kind=c_double) :: fraction
        end function geopm_prof_progress

        integer(kind=c_int) function geopm_prof_outer_sync(prof) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
        end function geopm_prof_outer_sync

        integer(kind=c_int) function geopm_prof_disable(prof, feature_name) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            character(kind=c_char), intent(in) :: feature_name(*)
        end function geopm_prof_disable

        integer(kind=c_int) function geopm_prof_print(prof, file_name, depth) bind(C)
            import
            implicit none
            type(c_ptr), intent(in) :: prof
            character(kind=c_char), intent(in) :: file_name(*)
            integer(kind=c_int), intent(in) :: depth
        end function geopm_prof_print

        !!!!!!!!!!!!!!!!!
        !! HELPER APIS !!
        !!!!!!!!!!!!!!!!!

        integer(kind=c_int) function geopm_num_node(comm, num_node)  bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: comm
            integer(kind=c_int) :: num_node(*)
        end function geopm_num_node

        integer(kind=c_int) function geopm_comm_split(comm, split_comm, is_ctl_comm) bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: comm
            integer(kind=c_int) :: split_comm(*)
            integer(kind=c_int) :: is_ctl_comm(*)
        end function geopm_comm_split

        integer(kind=c_int) function geopm_comm_split_ppn1(comm, ppn1_comm) bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: comm
            integer(kind=c_int) :: ppn1_comm(*)
        end function geopm_comm_split_ppn1

        integer(kind=c_int) function geopm_omp_sched_static_norm(num_iter, chunk_size, num_thread, norm) bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: num_iter
            integer(kind=c_int), intent(in) :: chunk_size
            integer(kind=c_int), intent(in) :: num_thread
            integer(kind=c_int) :: norm(*)
        end function geopm_omp_sched_static_norm

        integer(kind=c_int) function geopm_progress_threaded_min(num_iter, chunk_size, num_thread, min) bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: num_iter
            integer(kind=c_int), intent(in) :: chunk_size
            integer(kind=c_int), intent(in) :: num_thread
            integer(kind=c_int) :: min(*)
        end function geopm_progress_threaded_min

        integer(kind=c_int) function geopm_progress_threaded_sum(num_iter, chunk_size, num_thread, sum) bind(C)
            import
            implicit none
            integer(kind=c_int), intent(in) :: num_iter
            integer(kind=c_int), intent(in) :: chunk_size
            integer(kind=c_int), intent(in) :: num_thread
            integer(kind=c_int) :: sum(*)
        end function geopm_progress_threaded_sum

    end interface
end module geopm
