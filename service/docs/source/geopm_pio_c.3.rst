.. role:: raw-html-m2r(raw)
   :format: html


geopm_pio_c(3) -- interfaces to query and modify platform
=========================================================






SYNOPSIS
--------

#include `<geopm_pio.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_pio.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``int geopm_pio_num_signal_name(``\ :
  ``void);``

* 
  ``int geopm_pio_signal_name(``\ :
  ``int`` _name\ *idx*\ , :raw-html-m2r:`<br>`
  ``size_t`` _result\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_result_\ ``);``

* 
  ``int geopm_pio_num_control_name(``\ :
  ``void);``

* 
  ``int geopm_pio_control_name(``\ :
  ``int`` _name\ *index*\ , :raw-html-m2r:`<br>`
  ``size_t`` _result\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_result_\ ``);``

* 
  ``int geopm_pio_signal_description(``\ :
  `const char *`_signal\ *name*\ , :raw-html-m2r:`<br>`
  ``size_t`` _description\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_description_\ ``);``

* 
  ``int geopm_pio_control_description(``\ :
  `const char *`_control\ *name*\ , :raw-html-m2r:`<br>`
  ``size_t`` _description\ *max*\ , :raw-html-m2r:`<br>`
  `char *`_description_\ ``);``

* 
  ``int geopm_pio_signal_domain_type(``\ :
  `const char *`_signal\ *name*\ ``);``

* 
  ``int geopm_pio_control_domain_type(``\ :
  `const char *`_control\ *name*\ ``);``

* 
  ``int geopm_pio_read_signal(``\ :
  `const char *`_signal\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ , :raw-html-m2r:`<br>`
  `double *`_result_\ ``);``

* 
  ``int geopm_pio_write_control(``\ :
  `const char *`_control\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ , :raw-html-m2r:`<br>`
  ``double`` *setting*\ ``);``

* 
  ``int geopm_pio_save_control(``\ :
  ``void);``

* 
  ``int geopm_pio_restore_control(``\ :
  ``void);``

* 
  ``int geopm_pio_push_signal(``\ :
  `const char *`_signal\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``);``

* 
  ``int geopm_pio_push_control(``\ :
  `const char *`_control\ *name*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ , :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``);``

* 
  ``int geopm_pio_sample(``\ :
  ``int`` _signal\ *idx*\ , :raw-html-m2r:`<br>`
  `double *`_result_\ ``);``

* 
  ``int geopm_pio_adjust(``\ :
  ``int`` _control\ *idx*\ , :raw-html-m2r:`<br>`
  ``double`` *setting*\ ``);``

* 
  ``int geopm_pio_read_batch(``\ :
  ``void);``

* 
  ``int geopm_pio_write_batch(``\ :
  ``void);``

DESCRIPTION
-----------

The interfaces described in this man page are the C language bindings
for the `geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_ C++ class.  Please refer to the
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_ and `geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ man pages for
a general overview of the GEOPM platform interface layer.  The
`geopm_topo_c(3) <geopm_topo_c.3.html>`_ man page describes the C wrappers for the
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_ C++ class and documents the
``geopm_domain_e`` enum.  The caller selects from the ``geopm_domain_e``
enum for the _domain\ *type* parameter to many functions in the
geopm\ *pio*\ *() interface.  The return value from
``geopm_pio_signal_domain_type()`` and ``geopm_pio_control_domain_type()``
is also a value from the ``geopm_domain_e`` enum.

INSPECTION FUNCTIONS
--------------------


* 
  ``geopm_pio_num_signal_name``\ ():
  Returns the number of signal names that can be indexed with the
  _name\ *idx* parameter to the ``geopm_pio_signal_name()`` function.
  Any error in loading the platform will result in a negative error
  code describing the failure.

* 
  ``geopm_pio_signal_name``\ ():
  Sets the *result* string to the name of the signal indexed by
  _name\ *idx*.  At most _result\ *max* bytes are written to the
  *result* string.  The value of _name\ *idx* must be greater than
  zero and less than the return value from
  ``geopm_pio_num_signal_name()`` or else an error will occur.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be
  sufficient for storing any result.  If _result\ *max* is too small
  to contain the signal name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

* 
  ``geopm_pio_num_control_name``\ ():
  Returns the number of control names that can be indexed with the
  _name\ *idx* parameter to the ``geopm_pio_control_name()`` function.
  Any error in loading the platform will result in a negative error
  code describing the failure.

* 
  ``geopm_pio_control_name``\ ():
  Sets the *result* string to the name of the control indexed by
  _name\ *idx*.  At most _result\ *max* bytes are written to the
  *result* string.  The value of _name\ *idx* must be greater than
  zero and less than the return value from
  ``geopm_pio_num_control_name()`` or else an error will occur.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be
  sufficient for storing any result.  If _result\ *max* is too small
  to contain the control name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

* 
  ``geopm_pio_signal_description``\ ():
  Sets the *result* string to the signal description associated with
  _signal\ *name*.  At most _result\ *max* bytes are written to the
  *result* string.  Providing a string of ``NAME_MAX`` length (from
  ``limits.h``\ ) will be sufficient for storing any result.  If
  _result\ *max* is too small to contain the description an error will
  occur.  Zero is returned on success and a negative error code is
  returned if any error occurs.

* 
  ``geopm_pio_control_description``\ ():
  Sets the *result* string to the control description associated with
  _control\ *name*.  At most _result\ *max* bytes are written to the
  *result* string.  Providing a string of ``NAME_MAX`` length (from
  ``limits.h``\ ) will be sufficient for storing any result.  If
  _result\ *max* is too small to contain the description an error will
  occur.  Zero is returned on success and a negative error code is
  returned if any error occurs.

* 
  ``geopm_pio_signal_domain_type``\ ():
  Query the domain for the signal with name _signal\ *name*.  Returns
  one of the ``geopm_domain_e`` values signifying the granularity at
  which the signal is measured.  Will return a negative error code
  if any error occurs, e.g. a request for a _signal\ *name* that
  is not supported by the platform.

* 
  ``geopm_pio_control_domain_type``\ ():
  Query the domain for the control with the name _control\ *name*.
  Returns one of the ``geopm_domain_e`` values signifying the
  granularity at which the control can be adjusted.  Will return a
  negative error code if any error occurs, e.g. a request for a
  _control\ *name* that is not supported by the platform.

SERIAL FUNCTIONS
----------------


* 
  ``geopm_pio_read_signal``\ ():
  Read from the platform and interpret into SI units a signal
  associated with _signal\ *name* and store the value in *result*.
  This value is read from the ``geopm_topo_e`` _domain\ *type* domain
  indexed by _domain\ *idx*.  If the signal is native to a domain
  contained within _domain\ *type*\ , the values from the contained
  domains are aggregated to form *result*.  Calling this function
  does not modify values stored by calling ``geopm_pio_read_batch()``.
  If an error occurs then negative error code is returned.  Zero is
  returned upon success.

* 
  ``geopm_pio_write_control``\ ():
  Interpret the *setting* in SI units associated with _control\ *name*
  and write it to the platform.  This value is written to the
  ``geopm_topo_e`` _domain\ *type* domain indexed by _domain\ *idx*.  If
  the control is native to a domain contained within _domain\ *type*\ ,
  then the *setting* is written to all contained domains.  Calling
  this function does not modify values stored by calling
  ``geopm_pio_adjust()``.  If an error occurs then negative error code
  is returned.  Zero is returned upon success.

* 
  ``geopm_pio_save_control``\ ():
  Save the state of all controls so that any subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  may be reverted with a call to ``geopm_pio_restore_control()``.  The
  control settings are stored in memory managed by GEOPM.  If an
  error occurs then negative error code is returned.  Zero is
  returned upon success.

* 
  ``geopm_pio_restore_control``\ ():
  Restore the state recorded by the last call to
  ``geopm_pio_save_control()`` so that all subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  are reverted to their previous settings.  If an error occurs then
  negative error code is returned.  Zero is returned upon success.

BATCH FUNCTIONS
---------------


* 
  ``geopm_pio_push_signal``\ ():
  Push a signal onto the stack of batch access signals.  The signal
  is defined by selecting a _signal\ *name* from one of the values
  returned by the ``geopm_pio_signal_name()`` function, the
  _domain\ *type* from one of the ``geopm_domain_e`` values, and the
  _domain\ *idx* between zero to the value returned by
  ``geopm_topo_num_domain``\ (_domain\ *type*\ ).  Subsequent calls to the
  ``geopm_pio_read_batch()`` function will read the signal and update
  the internal state used to store batch signals.  The return value
  of ``geopm_pio_push_signal()`` is an index that can be passed as the
  _sample\ *idx* parameter to ``geopm_pio_sample()`` to access the
  signal value stored in the internal state from the last update.  A
  distinct signal index will be returned for each unique combination
  of input parameters.  All signals must be pushed onto the stack
  prior to the first call to ``geopm_pio_sample()`` or
  ``geopm_pio_read_batch()``.  Attempts to push a signal onto the
  stack after the first call to ``geopm_pio_sample()`` or
  ``geopm_pio_read_batch()`` or attempts to push a _signal\ *name* that
  is not a value provided by ``geopm_pio_signal_name()`` will result
  in a negative return value.

* 
  ``geopm_pio_push_control``\ ():
  Push a control onto the stack of batch access controls.  The
  control is defined by selecting a _control\ *name* from one of the
  values returned by the ``geopm_pio_control_name()`` function, the
  _domain\ *type* from one of the ``geopm_domain_e`` values, and the
  _domain\ *idx* between zero to the value returned by
  ``geopm_topo_num_domain``\ (_domain\ *type*\ ).  The return value of
  ``geopm_pio_push_control()`` can be passed to the
  ``geopm_pio_adjust()`` function which will update the internal state
  used to store batch controls.  Subsequent calls to the
  ``geopm_pio_write_batch()`` function access the control values in
  the internal state and write the values to the hardware.  A
  distinct control index will be returned for each unique
  combination of input parameters.  All controls must be pushed onto
  the stack prior to the first call to ``geopm_pio_adjust()`` or
  ``geopm_pio_write_batch()``.  Attempts to push a controls onto the
  stack after the first call to ``geopm_pio_adjust()`` or
  ``geopm_pio_write_batch()`` or attempts to push a _control\ *name*
  that is not a value provided by ``geopm_pio_control_name()`` will
  result in a negative return value.

* 
  ``geopm_pio_sample``\ ():
  Samples cached value of a single signal that has been pushed via
  ``geopm_pio_push_signal()`` and writes the value into *result*.  The
  _signal\ *idx* provided matches the return value from
  ``geopm_pio_push_signal()`` when the signal was pushed. The cached
  value is updated at the time of call to ``geopm_pio_read_batch()``.

* 
  ``geopm_pio_adjust``\ ():
  Updates cached value for single control that has been pushed via
  ``geopm_pio_push_control()`` to the value *setting*.  The
  _control\ *idx* provided matches the return value from
  ``geopm_pio_push_control()`` when the control was pushed.  The
  cached value will be written to the platform at time of call to
  ``geopm_pio_write_batch()``.

* 
  ``geopm_pio_read_batch``\ ():
  Read all push signals from the platform so that the next call to
  ``geopm_pio_sample()`` will reflect the updated data.

* 
  ``geopm_pio_write_batch``\ ():
  Write all pushed controls so that values provided to
  ``geopm_pio_adjust()`` are written to the platform.

RETURN VALUE
------------

If an error occurs in any call to an interface documented here, the
return value of the function will be a negative integer
corresponding to one of the error codes documented in
`geopm_error(3) <geopm_error.3.html>`_.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_topo_c(3) <geopm_topo_c.3.html>`_\ ,
`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3.html>`_\ ,
`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_
