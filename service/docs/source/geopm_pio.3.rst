
geopm_pio(3) -- interfaces to query and modify platform
=========================================================


Synopsis
--------

#include `<geopm_pio.h> <https://github.com/geopm/geopm/blob/dev/service/src/geopm_pio.h>`_\

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c

       int geopm_pio_num_signal_name(void);

       int geopm_pio_signal_name(int name_idx,
                                 size_t result_max,
                                 char *result);

       int geopm_pio_num_control_name(void);

       int geopm_pio_control_name(int name_index,
                                  size_t result_max,
                                  char *result);

       int geopm_pio_signal_description(const char *signal_name,
                                        size_t description_max,
                                        char *description);

       int geopm_pio_control_description(const char *control_name,
                                         size_t description_max,
                                         char *description);

       int geopm_pio_signal_domain_type(const char *signal_name);

       int geopm_pio_control_domain_type(const char *control_name);

       int geopm_pio_read_signal(const char *signal_name,
                                 int domain_type,
                                 int domain_idx,
                                 double *result);

       int geopm_pio_write_control(const char *control_name,
                                   int domain_type,
                                   int domain_idx,
                                   double setting);

       int geopm_pio_save_control(void);

       int geopm_pio_restore_control(void);

       int geopm_pio_save_control_dir(const char *save_dir);

       int geopm_pio_restore_control_dir(const char *save_dir);

       int geopm_pio_push_signal(const char *signal_name,
                                 int domain_type,
                                 int domain_idx);

       int geopm_pio_push_control(const char *control_name,
                                  int domain_type,
                                  int domain_idx);

       int geopm_pio_sample(int signal_idx,
                            double *result);

       int geopm_pio_adjust(int control_idx,
                            double setting);

       int geopm_pio_read_batch(void);

       int geopm_pio_write_batch(void);

       int geopm_pio_signal_info(const char *signal_name,
                                 int *aggregation_type,
                                 int *format_type,
                                 int *behavior_type);

       int geopm_pio_start_batch_server(int client_pid,
                                        int num_signal,
                                        const struct geopm_request_s *signal_config,
                                        int num_control,
                                        const struct geopm_request_s *control_config,
                                        int *server_pid,
                                        int key_size,
                                        char *server_key);

       int geopm_pio_stop_batch_server(int server_pid);

       int geopm_pio_format_signal(double signal,
                                   int format_type,
                                   size_t result_max,
                                   char *result);

       int geopm_pio_reset(void);

Description
-----------

The interfaces described in this man page are the **C** language bindings
for the :doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>` **C++** class.  Please refer to the
:doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>` and :doc:`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3>` man pages for
a general overview of the GEOPM platform interface layer.  The
:doc:`geopm_topo(3) <geopm_topo.3>` man page describes the **C** wrappers for the
:doc:`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3>` **C++** class and documents the
``geopm_domain_e`` enum.  The caller selects from the ``geopm_domain_e``
enum for the *domain_type* parameter to many functions in the
``geopm_pio_*()`` interface.  The return value from
``geopm_pio_signal_domain_type()`` and ``geopm_pio_control_domain_type()``
is also a value from the ``geopm_domain_e`` enum.

Inspection Functions
--------------------


``geopm_pio_num_signal_name()``
  Returns the number of signal names that can be indexed with the
  *name_idx* parameter to the ``geopm_pio_signal_name()`` function.
  Any error in loading the platform will result in a negative error
  code describing the failure.

``geopm_pio_signal_name()``
  Sets the *result* string to the name of the signal indexed by
  *name_idx*.  At most *result_max* bytes are written to the
  *result* string.  The value of *name_idx* must be greater than
  zero and less than the return value from
  ``geopm_pio_num_signal_name()`` or else an error will occur.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be
  sufficient for storing any *result*.  If *result_max* is too small
  to contain the signal name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

``geopm_pio_num_control_name()``
  Returns the number of control names that can be indexed with the
  *name_idx* parameter to the ``geopm_pio_control_name()`` function.
  Any error in loading the platform will result in a negative error
  code describing the failure.

``geopm_pio_control_name()``
  Sets the *result* string to the name of the control indexed by
  *name_idx*.  At most *result_max* bytes are written to the
  *result* string.  The value of *name_idx* must be greater than
  zero and less than the return value from
  ``geopm_pio_num_control_name()`` or else an error will occur.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be
  sufficient for storing any *result*.  If *result_max* is too small
  to contain the control name an error will occur.  Zero is returned
  on success and a negative error code is returned if any error
  occurs.

``geopm_pio_signal_description()``
  Sets the *description* string to the signal description associated with
  *signal_name*.  At most *description_max* bytes are written to the
  *description* string.  Providing a string of ``NAME_MAX`` length (from
  ``limits.h``\ ) will be sufficient for storing any description.  If
  *description_max* is too small to contain the description an error will
  occur.  Zero is returned on success and a negative error code is
  returned if any error occurs.

``geopm_pio_control_description()``
  Sets the *description* string to the control description associated with
  *control_name*.  At most *description_max* bytes are written to the
  *description* string.  Providing a string of ``NAME_MAX`` length (from
  ``limits.h``\ ) will be sufficient for storing any description.  If
  *description_max* is too small to contain the description an error will
  occur.  Zero is returned on success and a negative error code is
  returned if any error occurs.

``geopm_pio_signal_domain_type()``
  Query the domain for the signal with name *signal_name*.  Returns
  one of the ``geopm_domain_e`` values signifying the granularity at
  which the signal is measured.  Will return a negative error code
  if any error occurs, e.g. a request for a *signal_name* that
  is not supported by the platform.

``geopm_pio_control_domain_type()``
  Query the domain for the control with the name *control_name*.
  Returns one of the ``geopm_domain_e`` values signifying the
  granularity at which the control can be adjusted.  Will return a
  negative error code if any error occurs, e.g. a request for a
  *control_name* that is not supported by the platform.

``geopm_pio_signal_info()``
  **C** interface to get enums associated with a signal name.
  This interface supports **DBus** ``PlatformGetSignalInfo`` method.  This **C**
  interface is implemented using several ``PlatformIO`` methods unlike
  the other wrappers in this header.
  The parameters:
  **in** *signal_name*, name of signal to query.
  **out** *aggregation_type* One of the ``Agg::m_type_e`` enum values describing the way the signal is aggregated.
  **out** *format_type* One of the ``geopm::string_format_e`` enums defined in `Helper.hpp <https://github.com/geopm/geopm/blob/dev/service/src/geopm/Helper.hpp>`_\  that defines how to format
  the signal as a string.
  **out** *behavior_type* One of the ``IOGroup::m_signal_behavior_e`` enum values that decribes
  the signals behavior over time.
  Returns zero on success, error value on failure.

``geopm_pio_format_signal()``
  Format the *signal* according to the format type specified,
  and write the output into the *result* string.
  The parameters:
  **in** *signal* value to be formatted and written to the *result* string.
  **in** *format_type* One of the values in the ``string_format_e`` enum for specifying how to format the *signal* as a string.
  **in** *result_max* At most *result_max* bytes are written to the *result* string.
  If *result_max* is too small  to contain the written output string an error will occur.
  **out** *result* Sets the *result* string to the value of the signal formatted as specified by the *format_type*.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be sufficient for storing any *result*.
  Returns zero on success, error value on failure.

``geopm_pio_reset()``
Reset the GEOPM platform interface causing resources to be freed.
This will cause the internal PlatormIO instance to be
released/deleted and reconstructed.  As a result, any signals and
controls that had been pushed will be cleared, any batch servers
that had been started will be stopped, and all registered IOGroups
will be reset.  **NOTE: the reset only applies to the Service
PlatformIO instance and does not affect the PlatformIO instance
managed by the GEOPM HPC runtime.**


Serial Functions
----------------


``geopm_pio_read_signal()``
  Read from the platform and interpret into SI units a signal
  associated with *signal_name* and store the value in *result*.
  This value is read from the ``geopm_topo_e`` *domain_type* domain
  indexed by *domain_idx*.  If the signal is native to a domain
  contained within *domain_type*\ , the values from the contained
  domains are aggregated to form *result*.  Calling this function
  does not modify values stored by calling ``geopm_pio_read_batch()``.
  If an error occurs then negative error code is returned.  Zero is
  returned upon success.

``geopm_pio_write_control()``
  Interpret the *setting* in SI units associated with *control_name*
  and write it to the platform.  This value is written to the
  ``geopm_topo_e`` *domain_type* domain indexed by *domain_idx*.  If
  the control is native to a domain contained within *domain_type*\ ,
  then the *setting* is written to all contained domains.  Calling
  this function does not modify values stored by calling
  ``geopm_pio_adjust()``.  If an error occurs then negative error code
  is returned.  Zero is returned upon success.

``geopm_pio_save_control()``
  Save the state of all controls so that any subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  may be reverted with a call to ``geopm_pio_restore_control()``.  The
  control settings are stored in memory managed by GEOPM.  If an
  error occurs then negative error code is returned.  Zero is
  returned upon success.

``geopm_pio_restore_control()``
  Restore the state recorded by the last call to
  ``geopm_pio_save_control()`` so that all subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  are reverted to their previous settings.  If an error occurs then
  negative error code is returned.  Zero is returned upon success.

``geopm_pio_save_control_dir()``
  Save the state of all controls in the directory *save_dir* so that any subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  may be reverted with a call to ``geopm_pio_restore_control()``.  The
  control settings are stored in memory managed by GEOPM.  If an
  error occurs then negative error code is returned.  Zero is
  returned upon success.

``geopm_pio_restore_control_dir()``
  Restore the state recorded by the last call to
  ``geopm_pio_save_control()`` in the directory *save_dir* so that all subsequent changes made
  through ``geopm_pio_write_control()`` or ``geopm_pio_write_batch()``
  are reverted to their previous settings.  If an error occurs then
  negative error code is returned.  Zero is returned upon success.

When controls are saved, the data is stored in JSON format with the following
schema:

.. literalinclude:: ../../json_schemas/saved_controls.schema.json
    :language: json

Batch Functions
---------------


``geopm_pio_push_signal()``
  Push a signal onto the stack of batch access signals.  The signal
  is defined by selecting a *signal_name* from one of the values
  returned by the ``geopm_pio_signal_name()`` function, the
  *domain_type* from one of the ``geopm_domain_e`` values, and the
  *domain_idx* between zero to the value returned by
  ``geopm_topo_num_domain(domain_type)``.  Subsequent calls to the
  ``geopm_pio_read_batch()`` function will read the signal and update
  the internal state used to store batch signals.  The return value
  of ``geopm_pio_push_signal()`` is an index that can be passed as the
  *sample_idx* parameter to ``geopm_pio_sample()`` to access the
  signal value stored in the internal state from the last update.  A
  distinct signal index will be returned for each unique combination
  of input parameters.  All signals must be pushed onto the stack
  prior to the first call to ``geopm_pio_read_batch()`` or
  ``geopm_pio_adjust()``.  After calls to ``geopm_pio_read_batch()``
  or ``geopm_pio_adjust()`` have been made, signals may be pushed
  again only after performing a reset by calling ``geopm_pio_reset()``
  and before calling ``geopm_pio_read_batch()`` or
  ``geopm_pio_adjust()`` again.  Attempts to push a signal onto
  the stack after the first call to ``geopm_pio_read_batch()`` or
  ``geopm_pio_adjust()`` (and without performing a reset) or
  attempts to push a *signal_name* that is not a value provided by
  ``geopm_pio_signal_name()`` will result in a negative return value.

``geopm_pio_push_control()``
  Push a control onto the stack of batch access controls.  The
  control is defined by selecting a *control_name* from one of the
  values returned by the ``geopm_pio_control_name()`` function, the
  *domain_type* from one of the ``geopm_domain_e`` values, and the
  *domain_idx* between zero to the value returned by
  ``geopm_topo_num_domain(domain_type)``.  The return value of
  ``geopm_pio_push_control()`` can be passed to the
  ``geopm_pio_adjust()`` function which will update the internal state
  used to store batch controls.  Subsequent calls to the
  ``geopm_pio_write_batch()`` function access the control values in
  the internal state and write the values to the hardware.  A
  distinct control index will be returned for each unique
  combination of input parameters.  All controls must be pushed onto
  the stack prior to the first call to ``geopm_pio_adjust()`` or
  ``geopm_pio_read_batch()``.  After calls to ``geopm_pio_adjust()``
  or ``geopm_pio_read_batch()`` have been made, controls may be
  pushed again only after performing a reset by calling
  ``geopm_pio_reset()`` and before calling ``geopm_pio_adjust()`` or
  ``geopm_pio_read_batch()`` again.  Attempts to push a controls
  onto the stack after the first call to ``geopm_pio_adjust()`` or
  ``geopm_pio_read_batch()`` (and without performing a reset) or
  attempts to push a *control_name* that is not a value provided by
  ``geopm_pio_control_name()`` will result in a negative return value.

``geopm_pio_sample()``
  Samples cached value of a single signal that has been pushed via
  ``geopm_pio_push_signal()`` and writes the value into *result*.  The
  *signal_idx* provided matches the return value from
  ``geopm_pio_push_signal()`` when the signal was pushed. The cached
  value is updated at the time of call to ``geopm_pio_read_batch()``.

``geopm_pio_adjust()``
  Updates cached value for single control that has been pushed via
  ``geopm_pio_push_control()`` to the value *setting*.  The
  *control_idx* provided matches the return value from
  ``geopm_pio_push_control()`` when the control was pushed.  The
  cached value will be written to the platform at time of call to
  ``geopm_pio_write_batch()``.

``geopm_pio_read_batch()``
  Read all push signals from the platform so that the next call to
  ``geopm_pio_sample()`` will reflect the updated data.

``geopm_pio_write_batch()``
  Write all pushed controls so that values provided to
  ``geopm_pio_adjust()`` are written to the platform.

``geopm_pio_start_batch_server()``
  Creates a batch server with the following signals and controls.
  The list of signals is represented by the array *signal_config* of *num_signal* elements.
  The list of controls is represented by the array *control_config* of *num_control* elements.
  It would be an error to create a batch server without any signals or controls.
  In order to create a batch server, we also need to specify the PID of the client process as the *client_pid*.
  If the batch server is created successfully, it will populate the *server_pid* with the PID of the created server process,
  and the *server_key* with a key used to identify the server connection:
  a substring in interprocess shared memory keys used for communication.
  At most *key_size* bytes are written to the *server_key* string.
  Providing a string of ``NAME_MAX`` length (from ``limits.h``\ ) will be sufficient for storing any *server_key*.
  If *key_size* is too small to contain the *server_key* an error will
  occur.  Zero is returned on success and a negative error code is
  returned if any error occurs.

``geopm_pio_stop_batch_server()``
  This function is called directly by geopmd in order to
  end a batch session and kill the batch server process
  created by ``start_batch_server()``, which is the *server_pid* parameter.
  If the PID of the server process is not found, then it is an error,
  and this function returns a negative error code.
  Call through to ``BatchServer::stop_batch()``.

Return Value
------------

If an error occurs in any call to an interface documented here, the
return value of the function will be a negative integer
corresponding to one of the error codes documented in
:doc:`geopm_error(3) <geopm_error.3>`.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_topo(3) <geopm_topo.3>`\ ,
:doc:`geopm::PlatformTopo(3) <GEOPM_CXX_MAN_PlatformTopo.3>`\ ,
:doc:`geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3>`
