!
!  Copyright (c) 2015 - 2022, Intel Corporation
!  SPDX-License-Identifier: BSD-3-Clause


!> @defgroup fortran Fortran interfaces for GEOPM
module geopm_prof
    use, intrinsic :: ISO_C_BINDING
    implicit none

    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_UNKNOWN = 4294967296
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_COMPUTE = 8589934592
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_MEMORY =  17179869184
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_NETWORK = 34359738368
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_IO =      68719476736
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_SERIAL =  137438953472
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_PARALLEL =  274877906944
    INTEGER(8), PARAMETER :: GEOPM_REGION_HINT_IGNORE =  549755813888

    interface
        !!!!!!!!!!!!!!!!!!!!!!!!!!
        !! OBJECT INSTANTIATION !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!

        !> @brief Fortran interface to @link geopm_ctl.h geopm_ctl_create @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_ctl_create(policy, comm, ctl) bind(C, name="geopm_ctl_create_f")
            import
            implicit none
            type(c_ptr), value, intent(in) :: policy
            integer(kind=c_int), value, intent(in) :: comm
            type(c_ptr), intent(out) :: ctl
        end function geopm_ctl_create

        !> @brief Fortran interface to @link geopm_ctl.h geopm_ctl_destroy @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_ctl_destroy(ctl) bind(C)
            import
            implicit none
            type(c_ptr), value, intent(in) :: ctl
        end function geopm_ctl_destroy

        !!!!!!!!!!!!!!!!!!!!!!
        !! POWER MANAGEMENT !!
        !!!!!!!!!!!!!!!!!!!!!!

        !> @brief Fortran interface to @link geopm_ctl.h geopm_ctl_run @endlink C function.
        !> @ingroup fortran
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


        !!!!!!!!!!!!!!!!!!!!!!!!!!!
        !! APPLICATION PROFILING !!
        !!!!!!!!!!!!!!!!!!!!!!!!!!!

        !> @brief Fortran interface to @link geopm_prof.h geopm_prof_region @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_prof_region(region_name, hint, region_id) bind(C)
            import
            implicit none
            character(kind=c_char), intent(in) :: region_name(*)
            integer(kind=c_int64_t), value, intent(in) :: hint
            integer(kind=c_int64_t), intent(out) :: region_id
        end function geopm_prof_region

        !> @brief Fortran interface to @link geopm_prof.h geopm_prof_enter @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_prof_enter(region_id) bind(C)
            import
            implicit none
            integer(kind=c_int64_t), value, intent(in) :: region_id
        end function geopm_prof_enter

        !> @brief Fortran interface to @link geopm_prof.h geopm_prof_exit @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_prof_exit(region_id) bind(C)
            import
            implicit none
            integer(kind=c_int64_t), value, intent(in) :: region_id
        end function geopm_prof_exit

        !> @brief Fortran interface to @link geopm_prof.h geopm_prof_epoch @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_prof_epoch() bind(C)
            import
            implicit none
        end function geopm_prof_epoch

        !> @brief Fortran interface to @link geopm_prof.h geopm_tprof_init @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_tprof_init(num_work_unit) bind(C)
            import
            implicit none
            integer(kind=c_int32_t), value, intent(in) :: num_work_unit
        end function geopm_tprof_init

        !> @brief Fortran interface to @link geopm_prof.h geopm_tprof_post @endlink C function.
        !> @ingroup fortran
        integer(kind=c_int) function geopm_tprof_post() bind(C)
            import
            implicit none
        end function geopm_tprof_post

    end interface
end module geopm_prof
