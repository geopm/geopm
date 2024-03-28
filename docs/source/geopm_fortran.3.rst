
geopm_fortran(3) -- GEOPM fortran interface
===========================================


Synopsis
--------

``use geopm_prof``

Link with ``-lgeopmfort``


.. code-block:: fortran

       integer(kind=c_int) function geopm_ctl_create(policy, comm, ctl)
           type(c_ptr), value, intent(in) :: policy
           integer(kind=c_int), value, intent(in) :: comm
           type(c_ptr), intent(out) :: ctl
       end function geopm_ctl_create

       integer(kind=c_int) function geopm_ctl_destroy(ctl)
           type(c_ptr), value, intent(in) :: ctl
       end function geopm_ctl_destroy

       integer(kind=c_int) function geopm_ctl_run(ctl)
           type(c_ptr), value, intent(in) :: ctl
       end function geopm_ctl_run

       integer(kind=c_int) function geopm_prof_region(region_name, hint, region_id)
           character(kind=c_char), intent(in) :: region_name(*)
           integer(kind=c_int64_t), value, intent(in) :: hint
           integer(kind=c_int64_t), intent(out) :: region_id
       end function geopm_prof_region

       integer(kind=c_int) function geopm_prof_enter(region_id)
           integer(kind=c_int64_t), value, intent(in) :: region_id
       end function geopm_prof_enter

       integer(kind=c_int) function geopm_prof_exit(region_id)
           integer(kind=c_int64_t), value, intent(in) :: region_id
       end function geopm_prof_exit

       integer(kind=c_int) function geopm_prof_epoch()
       end function geopm_prof_epoch

       integer(kind=c_int) function geopm_tprof_init(num_work_unit)
           integer(kind=c_int32_t), value, intent(in) :: num_work_unit
       end function geopm_tprof_init

       integer(kind=c_int) function geopm_tprof_post()
       end function geopm_tprof_post

Description
-----------

This is the Fortran interface to the GEOPM library.  The documentation
for each function can found in the associated man page for the C
interface.

* :doc:`geopm_ctl(3) <geopm_ctl.3>`\

   * geopm_ctl_create
   * geopm_ctl_destroy
   * geopm_ctl_run

* :doc:`geopm_prof(3) <geopm_prof.3>`\

   * geopm_prof_region
   * geopm_prof_enter
   * geopm_prof_exit
   * geopm_prof_epoch
   * geopm_prof_init
   * geopm_prof_post



If the ``--disable-fortran`` configure flag is passed to the geopm
build then the Fortran interface will not be enabled.

Currently most of these Fortran functions call directly through to the
C interface through the ISO C bindings.  For this reason, care needs to
be taken when passing strings to the Fortran interface.  It is
important to wrap the Fortran string with ``char_c_`` and
``//c_null_char`` when passing to as follows:

.. code-block:: fortran

       ierr = geopm_prof_create(c_char_'profile_name'//c_null_char, c_char_''//c_null_char, MPI_COMM_WORLD, prof)

For interfaces that accept or return a MPI communicator, the
translation of the communicator from Fortran to C is done
transparently.  In the future similar techniques may be used for
converting Fortran strings.

Enum Type
---------

This fortran code has access to several enum values, which are defined in `geopm_hint.h <https://github.com/geopm/geopm/blob/dev/service/src/geopm_hint.h>`_\ :

* ``GEOPM_REGION_HINT_UNKNOWN``
* ``GEOPM_REGION_HINT_COMPUTE``
* ``GEOPM_REGION_HINT_MEMORY``
* ``GEOPM_REGION_HINT_NETWORK``
* ``GEOPM_REGION_HINT_IO``
* ``GEOPM_REGION_HINT_SERIAL``
* ``GEOPM_REGION_HINT_PARALLEL``
* ``GEOPM_REGION_HINT_IGNORE``
* ``GEOPM_REGION_HINT_INACTIVE``
* ``GEOPM_REGION_HINT_SPIN``

Fortran Module
--------------

The GEOPM package installs a Fortran 90 module file which defines
these interfaces and can be imported with the ``use geopm_prof`` command.
The install path for such modules has not been GNU standardized.  We
install the GEOPM Fortran 90 module to:

.. code-block::

       <LIBDIR>/<FC>/modules/geopm-<ARCH>/geopm.mod

where ``<LIBDIR>`` is the install location for libraries (e.g.
``/usr/lib64``\ ), ``<FC>`` is the Fortran compiler executable name
(e.g. gfortran), and ``<ARCH>`` is the processor architecture
(e.g. x86_64).

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_ctl(3) <geopm_ctl.3>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`
