.. role:: raw-html-m2r(raw)
   :format: html


geopm_time.h(3) -- helper methods for time
==========================================






SYNOPSIS
--------

#include `<geopm_time.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_time.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``int geopm_time_string(``\ :
  ``int`` _buf\ *size*\ , :raw-html-m2r:`<br>`
  `char *`_buf_\ ``);``

* 
  ``int geopm_time_to_string(``\ :
  `const struct geopm_time_s *`_time_, :raw-html-m2r:`<br>`
  ``int`` _buf\ *size*\ , :raw-html-m2r:`<br>`
  `char *`_buf_\ ``);``

* 
  ``int geopm_time(``\ :
  `struct geopm_time_s *`_time_\ ``);``

* 
  ``double geopm_time_diff(``\ :
  `const struct geopm_time_s *`_begin_, :raw-html-m2r:`<br>`
  `const struct geopm_time_s *`_end_\ ``);``

* 
  ``bool geopm_time_comp(``\ :
  `const struct geopm_time_s *`_aa_, :raw-html-m2r:`<br>`
  `const struct geopm_time_s *`_bb_\ ``);``

* 
  ``void geopm_time_add(``\ :
  `const struct geopm_time_s *`_begin_, :raw-html-m2r:`<br>`
  ``double`` *elapsed*\ , :raw-html-m2r:`<br>`
  `struct geopm_time_s *`_end_\ ``);``

* 
  ``double geopm_time_since(``\ :
  `const struct geopm_time_s *`_begin_\ ``);``

DESCRIPTION
-----------

The _geopm\ *time.h* header defines GEOPM interfaces for measuring time
in seconds relative to a fixed arbitrary reference point. The geopm_time_s
structure is used to hold time values.


* 
  ``geopm_time_string``\ ():
  Fills *buf* with the current date and time as a string.  The
  string will be null-terminated and truncated to _buf\ *size*\ , which
  must be at least 26 characters as required by `asctime_r(3) <http://man7.org/linux/man-pages/man3/asctime_r.3.html>`_.

* 
  ``geopm_time_to_string``\ ():
  Fills *buf* with the date and time indicated by the *time*
  structure converted to a string.  The string will be
  null-terminated and truncated to _buf\ *size*\ , which must be at
  least 26 characters as required by `asctime_r(3) <http://man7.org/linux/man-pages/man3/asctime_r.3.html>`_.

* 
  ``geopm_time``\ ():
  Sets *time* to the current time.

* 
  ``geopm_time_diff``\ ():
  Returns the difference in seconds between *begin* and *end*.

* 
  ``geopm_time_comp``\ ():
  Return true if *aa* is less than *bb*.

* 
  ``geopm_time_add``\ ():
  Sets *end* to *elapsed* seconds after *begin*.

* 
  ``geopm_time_since``\ ():
  Returns the number of seconds elapsed between the current time and *begin*.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`asctime_r(3) <http://man7.org/linux/man-pages/man3/asctime_r.3.html>`_
