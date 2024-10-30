
geopm::PlatformIO(3) -- GEOPM platform interface
================================================

Namespaces
----------

The ``PlatformIO`` class, the ``IOGroup`` class, and the ``platform_io()``
singleton accessor function are members of the ``namespace geopm``\ , but
the full names, ``geopm::PlatformIO``\ , ``geopm::IOGroup`` and
``geopm::platform_io()``\ , have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types:
``std::shared_ptr``\ , ``std::set``\ , ``std::string``\ , ``std::function``\ ,
``std::vector``\ , to enable better rendering of this manual.

Note that the ``PlatformIO`` class is an abstract base class that the
user interacts with.  The concrete implementation, ``PlatformIOImp``\ , is
hidden by the singleton accessor.

Synopsis
--------

#include `<geopm/PlatformIO.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm/PlatformIO.hpp>`_\

Link with ``-lgeopmd``


.. code-block:: c++

       PlatformIO &platform_io(void);

       PlatformIO::PlatformIO() = default;

       void PlatformIO::register_iogroup(shared_ptr<IOGroup> iogroup);

       set<string> PlatformIO::signal_names(void) const;

       set<string> PlatformIO::control_names(void) const;

       int PlatformIO::signal_domain_type(const string &signal_name) const;

       int PlatformIO::control_domain_type(const string &control_name) const;

       int PlatformIO::push_signal(const string &signal_name,
                                   int domain_type,
                                   int domain_idx);

       int PlatformIO::push_control(const string &control_name,
                                    int domain_type,
                                    int domain_idx);

       double PlatformIO::sample(int signal_idx);

       double PlatformIO::sample_combined(int signal_idx);

       void PlatformIO::adjust(int control_idx,
                               double setting);

       void PlatformIO::read_batch(void);

       void PlatformIO::write_batch(void);

       double PlatformIO::read_signal(const string &signal_name,
                                      int domain_type,
                                      int domain_idx);

       void PlatformIO::write_control(const string &control_name,
                                      int domain_type,
                                      int domain_idx,
                                      double setting);

       void PlatformIO::save_control(void);

       void PlatformIO::restore_control(void);

       void PlatformIO::save_control(const string &save_dir);

       void PlatformIO::restore_control(const string &save_dir);

       function<double(const vector<double> &)>
       PlatformIO::agg_function(const string &signal_name) const;

       function<string(double)>
       PlatformIO::format_function(const string &signal_name) const;

       string PlatformIO::signal_description(const string &signal_name) const;

       string PlatformIO::control_description(const string &control_name) const;

       int PlatformIO::signal_behavior(const string &signal_name) const;

       void PlatformIO::start_batch_server(int client_pid,
                                           const vector<geopm_request_s> &signal_config,
                                           const vector<geopm_request_s> &control_config,
                                           int &server_pid,
                                           string &server_key);

       void PlatformIO::stop_batch_server(int server_pid);

       static bool PlatformIO::is_valid_value(double value);


Description
-----------

The C++ bindings for the :doc:`geopm_pio(7) <geopm_pio.7>` interface.


Structure Type
--------------


This structure is part of the global C namespace. It contains information about a particular request,
a signal or a control, and all the relevant information such as the name of the request, the domain type,
and the domain index. This structure is used as a parameter to interface functions.

.. code-block:: c

       struct geopm_request_s {
           int domain_type;
           int domain_idx;
           char name[GEOPM_NAME_MAX];
       };

Singleton Accessor
------------------


``platform_io()``
  There is only one ``PlatformIO`` object, and the only way to access
  this object is through this function.  The function returns a
  reference to the single ``PlatformIO`` object that gives access to
  all of the `CLASS METHODS <INSPECTION CLASS METHODS_>`_ described below.  See `EXAMPLE <EXAMPLE_>`_ section
  below.

Static Class Methods
--------------------

``is_valid_value()``
  In geopm, the values for signals and controls can be invalid due to errors or absent sampling data.
  This function determines if the current *value* of the signal or control is
  a valid value. It returns true if the *value* is valid, and false if the *value*
  is not valid.


Inspection Class Methods
------------------------


``signal_names()``
  Returns the names of all available signals that can be requested.
  This includes all signals and aliases provided through ``IOGroup``
  extensions as well as signals provided by ``PlatformIO`` itself.  The
  set of strings that are returned can be passed as a *signal_name*
  to all ``PlatformIO`` methods that accept a *signal_name* input
  parameter.

``control_names()``
  Returns the names of all available controls.  This includes all
  controls and aliases provided by ``IOGroup``\ s as well as controls
  provided by ``PlatformIO`` itself.  The set of strings that are returned
  can be passed as a *control_name* to all ``PlatformIO`` methods that
  accept a *control_name* input parameter.

``signal_description()``
  Returns the description of the signal as defined by the ``IOGroup`` that
  provides this signal.

``control_description()``
  Returns the description of the control as defined by the ``IOGroup`` that
  provides this control.

``signal_domain_type()``
  Query the domain for the signal with name *signal_name*.  Returns
  one of the ``geopm_domain_e`` values signifying the
  granularity at which the signal is measured.  Will return
  ``GEOPM_DOMAIN_INVALID`` if the signal name is not
  supported.

``control_domain_type()``
  Query the domain for the control with the name *control_name*.
  Returns one of the ``geopm_domain_e`` values
  signifying the granularity at which the control can be adjusted.
  Will return ``GEOPM_DOMAIN_INVALID`` if the control
  name is not supported.

``agg_function()``
  Returns the function that should be used to aggregate
  *signal_name*.  If one was not previously specified by this class,
  the default function is select_first from :doc:`geopm::Agg(3) <geopm::Agg.3>`.

``signal_behavior()``
  Returns one of the ``IOGroup::m_signal_behavior_e`` values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

``format_function()``
  Returns a function that can be used to convert
  a signal of the given *signal_name* into a printable string.
  The returned function takes the *double* value of the signal and returns a formatted string.

Serial Class Methods
--------------------


``read_signal()``
  Read from the platform and interpret into SI units a signal
  given its name and domain.  Does not modify values stored by
  calling ``read_batch()``. The parameters correspond to the ``struct geopm_request_s``.
  The ``domain_type`` is from the ``enum geopm_domain_e`` described in `geopm_topo.h <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_topo.h>`_\

``write_control()``
  Interpret the setting and write it to the platform.  Does not
  modify the values stored by calling ``adjust()``.
  The first three parameters correspond to the ``struct geopm_request_s``.
  The ``domain_type`` is from the ``enum geopm_domain_e`` described in `geopm_topo.h <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_topo.h>`_
  ``setting`` is new value in SI units of the setting for the control.

``save_control()``
  Save the state of all controls so that any subsequent changes
  made through ``PlatformIO`` may be reverted with a call to
  ``restore_control()``. This function also has an overload which takes a *save_dir* parameter
  the directory to save the state of the control. Each ``IOGroup`` that supports controls
  will populate one file in the save directory
  that contains the saved state and name the file
  after the IOGroup name.

``restore_control()``
  Restore all controls to values recorded in previous call to
  ``save_control()``. This function also has an overload which takes a *save_dir* parameter
  the directory which contains the result of the previous saved state.

When controls are saved, the data is stored in JSON format with the following
schema:

.. literalinclude:: ../json_schemas/saved_controls.schema.json
    :language: json

Batch Class Methods
-------------------


``push_signal()``
  Push a signal onto the stack of batch access signals.  The signal
  is defined by selecting a *signal_name* from the set returned by
  the ``signal_names()`` method, the *domain_type* from one of the
  ``geopm_domain_e`` values, and the *domain_idx* between
  zero to the value returned by
  ``PlatformTopo::num_domain(domain_type)``.  Subsequent calls to
  the ``read_batch()`` method will read the signal and update the
  internal state used to store batch signals.  The return value of
  the method can be passed to the ``sample()`` method to access
  the signal stored in the internal state from the last update.  The
  returned signal index will be repeated for each unique tuple of
  push_signal input parameters.  All signals must be pushed onto the
  stack prior to the first call to ``sample()`` or ``read_batch()``.
  Attempts to push a signal onto the stack after the first call to
  ``sample()`` or ``read_batch()`` or attempts to push a *signal_name*
  that is not from the set returned by ``signal_names()`` will result
  in a thrown ``geopm::Exception`` with error number
  ``GEOPM_ERROR_INVALID``.

``push_control()``
  Push a control onto the stack of batch access controls.  The
  control is defined by selecting a *control_name* from the set
  returned by the ``control_names()`` method, the *domain_type* from
  one of the ``geopm_domain_e`` values, and the *domain_idx*
  between zero to the value returned by
  ``PlatformTopo::num_domain(domain_type)``.  The return value of
  the method can be passed to the ``adjust()`` method which will
  update the internal state used to store batch controls.
  Subsequent calls to the ``write_batch()`` method access the control
  values in the internal state and write the values to the hardware.
  The returned control index will be repeated for each unique tuple
  of ``push_control()`` input parameters.  All controls must be pushed
  onto the stack prior to the first call to ``adjust()`` or
  ``write_batch()``.  Attempts to push a controls onto the stack after
  the first call to ``adjust()`` or ``write_batch()`` or attempts to push
  a *control_name* that is not from the set returned by
  ``control_names()`` will result in a thrown ``geopm::Exception`` with
  error number ``GEOPM_ERROR_INVALID``.

``sample()``
  Samples cached value of a single signal that has been pushed via
  ``push_signal()``, which is identified by the *signal_idx*
  The cached value is updated at the time of call to
  ``read_batch()``, this function must be called after the update.
  The value of the signal is returned by the function.

``adjust()``
  Updates cached value for single control, which is the *setting*,
  that has been pushed via ``push_control()``, which is identified by the *control_idx*.
  The cached value will be written to the platform at
  time of call to ``write_batch()``.

``read_batch()``
  Read all pushed signals from the platform so that the next call to ``sample()``
  will reflect the updated data.

``write_batch()``
  Write all pushed controls so that values provided to ``adjust()``
  are written to the platform.

``start_batch_server()``
  Creates a batch server with the following signals and controls.
  The list of signals is represented by the vector *signal_config*.
  The list of controls is represented by the vector *control_config*.
  Both vectors are containing ``geopm_request_s`` objects.
  It would be an error to create a batch server with no signals and no controls.
  In order to create a batch server, we also need to specify the PID of the client process as the *client_pid*.
  If the batch server is created successfully, it will populate the *server_pid* with the PID of the created server process,
  and the *server_key* with a key used to identify the server connection:
  a substring in interprocess shared memory keys used for communication.
  An exception is thrown if any error occurs.

``geopm_pio_stop_batch_server()``
  This function is called directly by geopmd in order to
  end a batch session and kill the batch server process
  created by ``start_batch_server()``, which is the *server_pid* parameter.
  If the PID of the server process is not found, then it is an error,
  and this function throws an exception.
  Call through to ``BatchServer::stop_batch()``.


Plugin Class Methods
--------------------
``register_iogroup()``
  Registers an ``IOGroup`` with the ``PlatformIO`` so that the signals
  and controls provided by the object are available through the
  ``PlatformIO`` interface.  The *iogroup* is a shared pointer to a
  class derived from the :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`.  This method
  provides the mechanism for extending the ``PlatformIO`` interface at
  runtime.

Example
-------

.. code-block:: c++

   /* Print a signal for all CPUs on the system. */

   #include <iostream>
   #include <string>
   #include <geopm/PlatformIO.hpp>
   #include <geopm/PlatformTopo.hpp>

   int main(int argc, char **argv)
   {
       if (argc != 2) {
           std::cerr << "Usage: " << argv[0] << " SIGNAL_NAME" << std::endl;
           return -1;
       }
       std::string signal_name = argv[1];
       geopm::PlatformIO &pio = geopm::platform_io();
       geopm::PlatformTopo &topo = geopm::platform_topo();
       const int DOMAIN = pio.signal_domain_type(signal_name);
       const int NUM_DOMAIN = topo.num_domain(DOMAIN);
       std::cout << "cpu_idx    " << signal_name << std::endl;
       for (int domain_idx = 0; domain_idx != NUM_DOMAIN; ++domain_idx) {
           double signal = pio.read_signal(signal_name, DOMAIN, domain_idx);
           for (const auto &cpu_idx : topo.domain_cpus(DOMAIN, domain_idx)) {
               std::cout << cpu_idx << "    " << signal << std::endl;
           }
       }
       return 0;
   }



Errors
------

All functions described on this man page throw :doc:`geopm::Exception(3) <geopm::Exception.3>`
on error.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_hash(3) <geopm_hash.3>`\ ,
:doc:`geopm_prof(3) <geopm_prof.3>`\ ,
:doc:`geopm_pio(3) <geopm_pio.3>`\ ,
:doc:`geopm_topo(3) <geopm_topo.3>`\ ,
:doc:`geopm::Exception(3) <geopm::Exception.3>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopm::MSRIOGroup(3) <geopm::MSRIOGroup.3>`\ ,
:doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>`\ ,
:doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>`
