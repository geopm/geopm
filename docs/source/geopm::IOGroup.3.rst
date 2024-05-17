
geopm::IOGroup(3) -- provides system values and settings
========================================================

Namespaces
----------

The ``IOGroup`` class and the ``iogroup_factory()`` singleton accessor
function are members of the ``namespace geopm``\ , but the full names,
``geopm::IOGroup`` and ``geopm::iogroup_factory()``\ , have been abbreviated
in this manual.  Similarly, the ``std::`` namespace specifier has been
omitted from the interface definitions for the following standard
types: ``std::vector``\ , ``std::string``\ , ``std::set``\ , and ``std::function``\ ,
to enable better rendering of this manual.

Synopsis
--------

#include `<geopm/IOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm/IOGroup.hpp>`_\

Link with ``-lgeopmd``


.. code-block:: c++

       PluginFactory<IOGroup> &iogroup_factory(void);

       static vector<string> IOGroup::iogroup_names(void);

       static unique_ptr<IOGroup> IOGroup::make_unique(const string &iogroup_name);

       set<string> IOGroup::signal_names(void) const;

       set<string> IOGroup::control_names(void) const;

       bool IOGroup::is_valid_signal(const string &signal_name) const;

       bool IOGroup::is_valid_control(const string &control_name) const;

       int IOGroup::signal_domain_type(const string &signal_name) const;

       int IOGroup::control_domain_type(const string &control_name) const;

       int IOGroup::push_signal(const string &signal_name,
                                int domain_type,
                                int domain_idx);

       int IOGroup::push_control(const string &control_name,
                                 int domain_type,
                                 int domain_idx);

       void IOGroup::read_batch(void);

       void IOGroup::write_batch(void);

       double IOGroup::sample(int sample_idx);

       void IOGroup::adjust(int control_idx,
                            double setting);

       double IOGroup::read_signal(const string &signal_name,
                                   int domain_type,
                                   int domain_idx);

       void IOGroup::write_control(const string &control_name,
                                   int domain_type,
                                   int domain_idx,
                                   double setting);

       void IOGroup::save_control(void);

       void IOGroup::restore_control(void);

       function<double(const vector<double> &)> IOGroup::agg_function(const string &signal_name) const;

       function<string(double)> IOGroup::format_function(const string &signal_name) const;

       string IOGroup::signal_description(const string &signal_name) const;

       string IOGroup::control_description(const string &control_name) const;

       int IOGroup::signal_behavior(const string &signal_name) const;

       void IOGroup::save_control(const string &save_path);

       void IOGroup::restore_control(const string &save_path);

       static IOGroup::m_units_e IOGroup::string_to_units(const string &str);

       static string IOGroup::units_to_string(int);

       static IOGroup::m_signal_behavior_e IOGroup::string_to_behavior(const string &str);

Description
-----------

The ``IOGroup`` class is an abstract pure virtual class that defines the high
level interface employed by plugins for sample input and control output.  An
``Agent``\ , provided by the :doc:`geopm::Agent(3) <geopm::Agent.3>` class,  will define one or more
IOGroups in order to:


#. Acquire the necessary sample data required for the Agent.
#. Perform the necessary control operations as specified by the Agent.

Classes may derive from the ``IOGroup`` class in order to provide an ``Agent`` with
additional sample data or control hooks other than what is provided by GEOPM.
The pure virtual methods in this interface must be implemented by every
IOGroup.  If an IOGroup provides only signals, the methods related to controls
can have empty or degenerate implementations; the reverse is also true if an
IOGroup only provides controls.  In these cases, ensure that ``is_valid_signal()``
or ``is_valid_control()`` returns false as appropriate, and that ``signal_names()`` or
``control_names()`` returns an empty set.
GEOPM provides a number of built-in IOGroups for the most common
usages.  The list of built-in IOGroups is as follows:


*
  ``CpuinfoIOGroup``\ :
  Provides constants for CPU frequency limits.  Discussed in
  :doc:`geopm::CpuinfoIOGroup(3) <geopm::CpuinfoIOGroup.3>`.

*
  ``MSRIOGroup``\ :
  Provides signals and controls based on MSRs.  Discussed in
  :doc:`geopm::MSRIOGroup(3) <geopm::MSRIOGroup.3>`.

*
  ``ProfileIOGroup``\ :
  Provides signals from the application. Discussed in
  :doc:`geopm::ProfileIOGroup(3) <geopm::ProfileIOGroup.3>`.

*
  ``TimeIOGroup``\ :
  Provides a signal for the current time.  Discussed in
  :doc:`geopm::TimeIOGroup(3) <geopm::TimeIOGroup.3>`.

The APIs discussed in :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>` with regard to signals and
controls are ultimately fulfilled by the individual IOGroups that implement
this interface.

If multiple IOGroups define signals or controls that have the same name, the
IOGroup that is loaded last will override the others.  This effectively means
that the last loaded IOGroup that defines a signal or control will fulfill
requests for that signal or control.

Terms
-----

Below are some definitions of terms that are used to describe different parts
of the IOGroup interface.  Understanding these terms will help to interpret the
documentation about how to extend IOGroups.


*
  *signal*\ :
  Named parameter in SI units that can be measured.

*
  *control* :
  Named parameter in SI units that can be set.

*
  *domain*\ :
  The discrete component within a compute node where a signal or control is
  applicable.  For more information see :doc:`geopm::PlatformTopo(3) <geopm::PlatformTopo.3>`.

Factory Accessor
----------------


* ``iogroup_factory()``:
  This method returns the singleton accessor for the ``IOGroupFactory``.
  Calling this method will create the factory if it does not already exist.
  If this method is creating the factory, loading of the built-in IOGroups
  will be attempted.  For more information see :doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`
  and/or :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>`.

Class Methods
-------------


*
  ``iogroup_names()``:
  Provides the list of the IOGroups that are available in the factory.

*
  ``make_unique()``:
  Returns a ``unique_ptr`` to a new IOGroup object,
  uses the IOGroup factory to create an object of that type.

*
  ``signal_names()``:
  Provides the list of all signals provided by the ``IOGroup``.  The
  set of strings that are returned can be passed as a ``signal_name``
  to all of the ``IOGroup`` methods that accept a ``signal_name`` as an
  input parameter.

*
  ``control_names()``:
  Provides the list of all controls provided by the ``IOGroup``.  The set of
  strings that are returned can be passed as a ``control_name`` to all of the
  ``IOGroup`` methods that accept a ``control_name`` as an input parameter.

*
  ``is_valid_signal()``:
  Tests if the *signal_name* refers to a signal supported by the
  ``IOGroup``.

*
  ``is_valid_control()``:
  Test if the *control_name* refers to a control supported by the
  ``IOGroup``.

*
  ``signal_domain_type()``:
  Query the domain for a named signal.

*
  ``control_domain_type()``:
  Query the domain for a named control.

*
  ``push_signal()``:
  Add a signal to the list of signals that is read by ``read_batch()``
  and sampled by ``sample()``.  This method should return a unique index
  for each signal that can be utilized when calling ``sample()``.

*
  ``push_control()``:
  Add a control to the list of controls that is written by
  ``write_batch()`` and configured with ``adjust()``.  This method should
  return a unique index for each control that can be utilized when calling
  ``control()``.

*
  ``read_batch()``:
  Read all pushed signals from the platform so that the next call to
  ``sample()`` will reflect the updated data.  The intention is that
  ``read_batch()`` will read the all of the ``IOGroup``\ 's signals into memory once
  per call.

*
  ``write_batch()``:
  Write all of the pushed controls so that values previously given
  to ``adjust()`` are written to the platform.

*
  ``sample()``:
  Retrieve a signal value from the data read by the last call to
  ``read_batch()`` for a particular signal previously pushed with
  ``push_signal()``.

*
  ``adjust()``:
  Adjust a setting for a particular control that was previously
  pushed with ``push_control()``. This adjustment will be written to
  the platform on the next call to ``write_batch()``.

*
  ``read_signal()``:
  Read from platform and interpret into SI units a signal given its
  name and domain. Does *not* modify the values stored by calling
  ``read_batch()``.

*
  ``write_control()``:
  Interpret the setting and write setting to the platform.  Does *not*
  modify the values stored by calling ``adjust()``.

*
  ``save_control()``:
  Save the state of all controls so that any subsequent changes made
  through the IOGroup can be undone with a call to the ``restore()`` method.
  Also has an overloaded version which takes the *save_path*.

*
  ``restore_control()``:
  Restore all controls to values recorded in previous call to the ``save()`` method.
  Also has an overloaded version which takes the *save_path*.

*
  ``agg_function()``:
  Returns a function that should be used when aggregating a signal
  of the type *signal_name*.  For more information see
  :doc:`geopm::Agg(3) <geopm::Agg.3>`.

*
  ``format_function()``:
  Returns a function that can be used to convert a signal of the
  type *signal_name* into a human readable string representation.

*
  ``signal_description()``:
  Returns a description of the signal. This string can be used by
  tools to generate help text for users of the IOGroup.

*
  ``control_description()``:
  Returns a description of the control. This string can be used by
  tools to generate help text for users of the IOGroup.

*
  ``signal_behavior()``:
  Returns one of the ``IOGroup::signal_behavior_e`` values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

*
  ``string_to_units()``:
  Convert a ``string`` to the corresponding ``m_units_e`` value

*
  ``units_to_string()``:
  Convert the ``m_units_e`` value to the corresponding ``string``.

*
  ``string_to_behavior()``:
  Convert a ``string`` to the corresponding ``m_signal_behavior_e`` value

Example
-------

Please see the `GEOPM IOGroup
tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/iogroup>`_ for more
information.  That code is located in the GEOPM source under tutorial/iogroup.

Further documentation for this module can be found in the
`doxygen page <https://geopm.github.io/geopm-service-dox/classgeopm_1_1_i_o_group.html>`_.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agg(3) <geopm::Agg.3>`\ ,
:doc:`geopm::CpuinfoIOGroup(3) <geopm::CpuinfoIOGroup.3>`\ ,
:doc:`geopm::MSRIOGroup(3) <geopm::MSRIOGroup.3>`\ ,
:doc:`geopm::PlatformIO(3) <geopm::PlatformIO.3>`\ ,
:doc:`geopm::TimeIOGroup(3) <geopm::TimeIOGroup.3>`
