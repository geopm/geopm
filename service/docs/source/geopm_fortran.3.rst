.. role:: raw-html-m2r(raw)
   :format: html


geopm_fortran(3) -- geopm fortran interface
===========================================






SYNOPSIS
--------

``use geopm``

``Link with -lgeopmfort``


* 
  `integer(kind=c_int) function geopm_ctl_create(`_policy_\ ``,`` _shm\ *key*\ ``,`` *comm*\ ``,`` *ctl*\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *policy* :raw-html-m2r:`<br>`
  ``character(kind=c_char), intent(in) ::`` _sample\ *key*\ ``(*)`` :raw-html-m2r:`<br>`
  ``integer(kind=c_int), value, intent(in) ::`` *comm* :raw-html-m2r:`<br>`
  ``type(c_ptr), intent(out) ::`` *ctl*

* 
  `integer(kind=c_int) function geopm_ctl_destroy(`_ctl_\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *ctl* :raw-html-m2r:`<br>`

* 
  `integer(kind=c_int) function geopm_ctl_run(`_ctl_\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *ctl*

* 
  `integer(kind=c_int) function geopm_prof_create(`_prof\ *name*\ ``,`` _shm\ *key*\ ``,`` *comm*\ ``,`` *prof*\ ``)``\ :
  ``character(kind=c_char), intent(in) ::`` _prof\ *name*\ ``(*)`` :raw-html-m2r:`<br>`
  ``character(kind=c_char), intent(in) ::`` _shm\ *key*\ ``(*)`` :raw-html-m2r:`<br>`
  ``integer(kind=c_int), value, intent(in) ::`` *comm* :raw-html-m2r:`<br>`
  ``type(c_ptr), intent(out) ::`` *prof*

* 
  `integer(kind=c_int) function geopm_prof_destroy(`_prof_\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof*

* 
  `integer(kind=c_int) function geopm_prof_default(`_prof_\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof*

* 
  `integer(kind=c_int) function geopm_prof_region(`_prof_\ ``,`` _region\ *name*\ ``,`` _policy\ *hint*\ ``,`` _region\ *id*\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof* :raw-html-m2r:`<br>`
  ``character(kind=c_char), intent(in) ::`` _region\ *name*\ ``(*)`` :raw-html-m2r:`<br>`
  ``integer(kind=c_int), value, intent(in) ::`` _policy\ *hint* :raw-html-m2r:`<br>`
  ``integer(kind=c_int64_t), intent(out) ::`` _region\ *id*

* 
  `integer(kind=c_int) function geopm_prof_enter(`_prof_\ ``,`` _region\ *id*\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof* :raw-html-m2r:`<br>`
  ``integer(kind=c_int64_t), value, intent(in) ::`` _region\ *id*

* 
  `integer(kind=c_int) function geopm_prof_exit(`_prof_\ ``,`` _region\ *id*\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof* :raw-html-m2r:`<br>`
  ``integer(kind=c_int64_t), value, intent(in) ::`` _region\ *id*

* 
  `integer(kind=c_int) function geopm_prof_epoch(`_prof_\ ``)``\ :
  ``type(c_ptr), value, intent(in) ::`` *prof*

* 
  `integer(kind=c_int) function geopm_tprof_init(`_num_work\ *unit*\ ``)``\ :
  ``integer(kind=c_int32_t), value, intent(in) ::`` _num_work\ *unit* :raw-html-m2r:`<br>`

* 
  ``integer(kind=c_int) function geopm_tprof_post()``\ :

DESCRIPTION
-----------

This is the Fortran interface to the GEOPM library.  The documentation
for each function can found in the associated man page for the C
interface.  If the ``--disable-fortran`` configure flag is passed to the geopm
build then the Fortran interface will not be enabled.

Currently most of these Fortran functions call directly through to the
C interface through the ISO C bindings.  For this reason, care needs to
be taken when passing strings to the Fortran interface.  It is
important to wrap the Fortran string with ``char_c_`` and
``//c_null_char`` when passing to as follows:

``ierr = geopm_prof_create(c_char_'profile_name'//c_null_char, c_char_''//c_null_char, MPI_COMM_WORLD, prof)``

For interfaces that accept or return a MPI communicator, the
translation of the communicator from Fortran to C is done
transparently.  In the future similar techniques may be used for
converting Fortran strings.

FORTRAN MODULE
--------------

The GEOPM package installs a Fortran 90 module file which defines
these interfaces and can be imported with the ``use geopm`` command.
The install path for such modules has not been GNU standardized.  We
install the geopm Fortran 90 module to:

``<LIBDIR>/<FC>/modules/geopm-<ARCH>/geopm.mod``

where ``<LIBDIR>`` is the install location for libraries (e.g.
``/usr/lib64``\ ), ``<FC>`` is the Fortran compiler executable name
(e.g. gfortran), and ``<ARCH>`` is the processor architecture
(e.g. x86_64).

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_ctl_c(3) <geopm_ctl_c.3.html>`_\ ,
`geopm_error(3) <geopm_error.3.html>`_\ ,
`geopm_prof_c(3) <geopm_prof_c.3.html>`_
