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

        !> @brief Fortran interface to geopm_ctl_create C function.
        integer(kind=c_int) function geopm_ctl_create(policy, comm, ctl) bind(C, name="geopm_ctl_create_f")
            import
            implicit none
            type(c_ptr), value, intent(in) :: policy
            integer(kind=c_int), value, intent(in) :: comm
            type(c_ptr), intent(out) :: ctl
        end function geopm_ctl_create

        !> @brief Fortran interface to geopm_ctl_destroy C function.
        integer(kind=c_int) function geopm_ctl_destroy(ctl) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: ctl
        end function geopm_ctl_destroy

        !!!!!!!!!!!!!!!!!!!!!!
        !! POWER MANAGEMENT !!
        !!!!!!!!!!!!!!!!!!!!!!

        !> @brief Fortran interface to geopm_ctl_step C function.
        integer(kind=c_int) function geopm_ctl_step(ctl) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: ctl
        end function geopm_ctl_step

        !> @brief Fortran interface to geopm_ctl_run C function.
        integer(kind=c_int) function geopm_ctl_run(ctl) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: ctl
        end function geopm_ctl_run

!        integer(kind=c_int) function geopm_ctl_pthread(ctl, attr, thread) bind(C)
!            import
!            implicit none
!            type(c_ptr), intent(in) :: ctl
!            type(pthread_attr), intent(in) :: attr(*)
!            type(pthread_t), intent(in) :: thread(*)
!        end function geopm_ctl_pthread


        !> @brief Fortran interface to geopm_ctl_spawn C function.
        integer(kind=c_int) function geopm_ctl_spawn(ctl) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: ctl
        end function geopm_ctl_spawn

        !!!!!!!!!!!!!!!!!!!!!!!!!!!
        !! APPLICATION PROFILING !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!!

        !> @brief Fortran interface to geopm_prof_region C function.
        integer(kind=c_int) function geopm_prof_region(region_name, policy_hint, region_id) bind(C)
            import
            implicit none
            character(kind=c_char), intent(in) :: region_name(*)
            integer(kind=c_int), value, intent(in) :: policy_hint
            integer(kind=c_int64_t), intent(out) :: region_id
        end function geopm_prof_region

        !> @brief Fortran interface to geopm_prof_enter C function.
        integer(kind=c_int) function geopm_prof_enter(region_id) bind(C)
            import
            implicit none
            integer(kind=c_int64_t), value, intent(in) :: region_id
        end function geopm_prof_enter

        !> @brief Fortran interface to geopm_prof_exit C function.
        integer(kind=c_int) function geopm_prof_exit(region_id) bind(C)
            import
            implicit none
            integer(kind=c_int64_t), value, intent(in) :: region_id
        end function geopm_prof_exit

        !> @brief Fortran interface to geopm_prof_progress C function.
        integer(kind=c_int) function geopm_prof_progress(region_id, fraction) bind(C)
            import
            implicit none
            integer(kind=c_int64_t), value, intent(in) :: region_id
            real(kind=c_double), value, intent(in) :: fraction
        end function geopm_prof_progress

        !> @brief Fortran interface to geopm_prof_epoch C function.
        integer(kind=c_int) function geopm_prof_epoch() bind(C)
            import
            implicit none
        end function geopm_prof_epoch

        !> @brief Fortran interface to geopm_prof_disable C function.
        integer(kind=c_int) function geopm_prof_disable(feature_name) bind(C)
            import
            implicit none
            character(kind=c_char), intent(in) :: feature_name(*)
        end function geopm_prof_disable

        !> @brief Fortran interface to geopm_tprof_create C function.
        integer(kind=c_int) function geopm_tprof_create(num_thread, num_iter, chunk_size, tprof) bind(C)
            import
            implicit none
            integer(kind=c_int), value, intent(in) :: num_thread
            integer(kind=c_size_t), value, intent(in) :: num_iter
            integer(kind=c_size_t), value, intent(in) :: chunk_size
            type(c_ptr), intent(out) :: tprof
        end function geopm_tprof_create

        !> @brief Fortran interface to geopm_tprof_destroy C function.
        integer(kind=c_int) function geopm_tprof_destroy(tprof) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: tprof
        end function geopm_tprof_destroy

        !> @brief Fortran interface to geopm_tprof_increment C function.
        integer(kind=c_int) function geopm_tprof_increment(tprof, region_id, thread_idx) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: tprof
            integer(kind=c_int64_t), value, intent(in) :: region_id
            integer(kind=c_int), value, intent(in) :: thread_idx
        end function geopm_tprof_increment

    end interface
end module geopm
