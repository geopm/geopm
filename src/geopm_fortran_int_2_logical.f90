      module geopm_logical
      use iso_c_binding
      implicit none

      public :: c_int_to_f_logical

      contains

      function c_int_to_f_logical(cb) bind(c)
          use iso_c_binding, only: c_int
          logical c_int_to_f_logical
          integer(c_int) :: cb
          c_int_to_f_logical = (cb .ne. 0)
      end function c_int_to_f_logical
      end module
