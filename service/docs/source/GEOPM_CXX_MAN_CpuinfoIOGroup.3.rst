
geopm::CpuinfoIOGroup(3) -- IOGroup for CPU frequency limits
============================================================


Namespaces
----------

The ``CpuinfoIOGroup`` class and the ``IOGroup`` class are members of
the ``namespace geopm``, but the full names, ``geopm::CpuinfoIOGroup`` and
``geopm::IOGroup``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

Synopsis
--------

#include `<geopm/CpuinfoIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/service/src/CpuinfoIOGroup.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       set<string> CpuinfoIOGroup::signal_names(void) const override;

       set<string> CpuinfoIOGroup::control_names(void) const override;

       bool CpuinfoIOGroup::is_valid_signal(const string &signal_name) const override;

       bool CpuinfoIOGroup::is_valid_control(const string &control_name) const override;

       int CpuinfoIOGroup::signal_domain_type(const string &signal_name) const override;

       int CpuinfoIOGroup::control_domain_type(const string &control_name) const override;

       int CpuinfoIOGroup::push_signal(const string &signal_name, int domain_type, int domain_idx) override;

       int CpuinfoIOGroup::push_control(const string &control_name, int domain_type, int domain_idx) override;

       void CpuinfoIOGroup::read_batch(void) override;

       void CpuinfoIOGroup::write_batch(void) override;

       double CpuinfoIOGroup::sample(int batch_idx) override;

       void CpuinfoIOGroup::adjust(int batch_idx, double setting) override;

       double CpuinfoIOGroup::read_signal(const string &signal_name, int domain_type, int domain_idx) override;

       void CpuinfoIOGroup::write_control(const string &control_name, int domain_type, int domain_idx, double setting) override;

       void CpuinfoIOGroup::save_control(void) override;

       void CpuinfoIOGroup::restore_control(void) override;

       function<double(const vector<double> &)> CpuinfoIOGroup::agg_function(const string &signal_name) const override;

       function<string(double)> CpuinfoIOGroup::format_function(const string &signal_name) const override;

       string CpuinfoIOGroup::signal_description(const string &signal_name) const override;

       string CpuinfoIOGroup::control_description(const string &control_name) const override;

       int CpuinfoIOGroup::signal_behavior(const string &signal_name) const override;

       void CpuinfoIOGroup::save_control(const string &save_path) override;

       void CpuinfoIOGroup::restore_control(const string &save_path) override;

       static CpuinfoIOGroup::string plugin_name(void);

       static unique_ptr<IOGroup> CpuinfoIOGroup::make_plugin(void);

Description
-----------

The ``CpuinfoIOGroup`` provides constants for CPU frequency limits as
signals.  These values are obtained through the `proc(5) <https://man7.org/linux/man-pages/man5/proc.5.html>`_ filesystem.

Class Methods
-------------


*
  ``signal_names()``:
  Returns the list of signal names provided by this IOGroup.

*
  ``control_names()``:
  Does nothing; this IOGroup does not provide any controls.

*
  ``is_valid_signal()``:
  Returns whether the given *signal_name* is supported by the
  ``CpuinfoIOGroup`` for the current platform.

*
  ``is_valid_control()``:
  Returns false; this IOGroup does not provide any controls.

*
  ``signal_domain_type()``:
  If the *signal_name* is valid for this IOGroup, returns ``GEOPM_DOMAIN_BOARD``.
  All constants provided by the ``CpuinfoIOGroup`` are assumed to be the same for the whole board.

*
  ``control_domain_type()``:
  Returns ``GEOPM_DOMAIN_INVALID``; this IOGroup does not provide any controls.

*
  ``push_signal()``:
  Adds the signal specified by *signal_name* to the list of signals
  to be read during ``read_batch()``.  If *domain_type* is not
  ``GEOPM_DOMAIN_BOARD``, throws an error.  The *domain_idx* is ignored.

*
  ``push_control()``:
  Should not be called; this IOGroup does not provide any controls.
  throws an error

*
  ``read_batch()``:
  Does nothing; this IOGroup's signals are constant.

*
  ``write_batch()``:
  Does nothing; this IOGroup does not provide any controls.

*
  ``sample()``:
  Returns the value of the signal specified by a *signal_idx*
  returned from ``push_signal()``.  The value will have been updated by
  the most recent call to ``read_batch()``.

*
  ``adjust()``:
  Should not be called; this IOGroup does not provide any controls.
  throws an error

*
  ``read_signal()``:
  Immediately return the stored value for the given *signal_name*.
  If *domain_type* is not ``GEOPM_DOMAIN_BOARD``, throws an error.  The *domain_idx*
  is ignored.

*
  ``write_control()``:
  Should not be called; this IOGroup does not provide any controls.
  throws an error

*
  ``save_control()``:
  Does nothing; this IOGroup does not provide any controls.

*
  ``restore_control()``:
  Does nothing; this IOGroup does not provide any controls.

*
  ``agg_function()``:
  For all valid signals in this IOGroup, the aggregation function is
  ``expect_same()``, described in :doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`.  If any frequency
  range constants are compared between nodes, they should be the
  same or the runtime may behave unpredictably.

*
  ``format_function()``:
  Return a function that should be used when formatting the given
  signal.  For more information see :doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`.

*
  ``signal_description()``:
  Returns a string description for *signal_name*, if defined.

*
  ``control_description()``:
  Does nothing; this IOGroup does not provide any controls.
  Returns an empty string.

*
  ``signal_behavior()``:
  Returns one of the ``IOGroup::signal_behavior_e`` values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

*
  ``plugin_name()``:
  Returns the name of the plugin to use when this plugin is
  registered with the IOGroup factory; see
  :doc:`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3>` for more details.

*
  ``make_plugin()``:
  Returns a pointer to a new CpuinfoIOGroup object; see
  :doc:`geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3>` for more details.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
`proc(5) <https://man7.org/linux/man-pages/man5/proc.5.html>`_\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`
