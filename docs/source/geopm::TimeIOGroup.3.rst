
geopm::TimeIOGroup(3) -- IOGroup providing time signals
=======================================================


Namespaces
----------

The ``TimeIOGroup`` class and the ``IOGroup`` class are members of the ``namespace geopm``\ , but
the full names, ``geopm::TimeIOGroup`` and ``geopm::IOGroup``, have been abbreviated in this
manual.  Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::set``\ , ``std::unique_ptr``\ , and ``std::function``\ , to enable better rendering of
this manual.


Synopsis
--------

#include `<geopm/TimeIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/TimeIOGroup.hpp>`_

Link with ``-lgeopmd``


.. code-block:: c++

       set<string> TimeIOGroup::signal_names(void) const override;

       set<string> TimeIOGroup::control_names(void) const override;

       bool TimeIOGroup::is_valid_signal(const string &signal_name) const override;

       bool TimeIOGroup::is_valid_control(const string &control_name) const override;

       int TimeIOGroup::signal_domain_type(const string &signal_name) const override;

       int TimeIOGroup::control_domain_type(const string &control_name) const override;

       int TimeIOGroup::push_signal(const string &signal_name,
                                    int domain_type,
                                    int domain_idx) override;

       int TimeIOGroup::push_control(const string &control_name,
                                     int domain_type,
                                     int domain_idx) override;

       void TimeIOGroup::read_batch(void) override;

       void TimeIOGroup::write_batch(void) override;

       double TimeIOGroup::sample(int batch_idx) override;

       void TimeIOGroup::adjust(int batch_idx,
                                double setting) override;

       double TimeIOGroup::read_signal(const string &signal_name,
                                       int domain_type,
                                       int domain_idx) override;

       void TimeIOGroup::write_control(const string &control_name,
                                       int domain_type,
                                       int domain_idx,
                                       double setting) override;

       void TimeIOGroup::save_control(void) override;

       void TimeIOGroup::restore_control(void) override;

       function<double(const vector<double> &)> TimeIOGroup::agg_function(const string &signal_name) const override;

       function<string(double)> TimeIOGroup::format_function(const string &signal_name) const override;

       string TimeIOGroup::signal_description(const string &signal_name) const override;

       string TimeIOGroup::control_description(const string &control_name) const override;

       int TimeIOGroup::signal_behavior(const string &signal_name) const override;

       void TimeIOGroup::save_control(const string &save_path) override;

       void TimeIOGroup::restore_control(const string &save_path) override;

       string TimeIOGroup::name(void) const override;

       static string TimeIOGroup::plugin_name(void);

       static unique_ptr<IOGroup> TimeIOGroup::make_plugin(void);

Description
-----------

The ``TimeIOGroup`` class is a derived implementation of :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`
that provides an implementation of the ``TIME`` signal for the time since GEOPM startup.

Class Methods
-------------


``signal_names()``
  Returns the time signal name, ``"TIME::ELAPSED"``, and its alias, ``"TIME"``.

``control_names()``
  Does nothing; this ``IOGroup`` does not provide any controls.

``is_valid_signal()``
  Returns ``true`` if the *signal_name* is one from the list returned by
  ``signal_names()``.

``is_valid_control()``
  Returns ``false``; this ``IOGroup`` does not provide any controls.

``signal_domain_type()``
  If the *signal_name* is valid for this ``IOGroup``, returns
  ``GEOPM_DOMAIN_CPU``, returns ``GEOPM_DOMAIN_INVALID``.

``control_domain_type()``
  Returns ``GEOPM_DOMAIN_INVALID``; this ``IOGroup`` does not provide any controls.

``push_signal()``
  Since this ``IOGroup`` only provides one signal, returns ``0`` if the *signal_name*
  is valid. Throws a variety of exceptions if the parameters do not check out.
  The *domain_idx* parameter is ignored.

``push_control()``
  Should not be called; this ``IOGroup`` does not provide any controls.
  Throws an exception always.

``read_batch()``
  If a time signal has been pushed, updates the time since the
  ``TimeIOGroup`` was created.

``write_batch()``
  Does nothing; this ``IOGroup`` does not provide any controls.

``sample()``
  Returns the value of the signal specified by a *batch_idx*
  returned from ``push_signal()``.  The value will have been updated by
  the most recent call to ``read_batch()``.
  Throws a variety of exceptions to distinguish between error conditions.

``adjust()``
  Should not be called; this ``IOGroup`` does not provide any controls.
  Throws an exception always.

``read_signal()``
  If *signal_name* is valid, immediately return the time since the
  ``TimeIOGroup`` was created.
  Throws a variety of exceptions if the parameters do not check out.
  The *domain_idx* parameter is ignored.

``write_control()``
  Should not be called; this ``IOGroup`` does not provide any controls.
  Throws an exception always.

``save_control()``
  This function also has an overload form that takes the *save_path* parameter.
  Does nothing in both of its forms; this ``IOGroup`` does not provide any controls.

``restore_control()``
  This function also has an overload form that takes the *save_path* parameter.
  Does nothing in both of its forms; this ``IOGroup`` does not provide any controls.

``agg_function()``
  The ``TIME`` signal provided by this ``IOGroup`` is aggregated using the
  ``average()`` function from :doc:`geopm::Agg(3) <geopm::Agg.3>`.
  Throws an exception if the *signal_name* is invalid.

``format_function()``
  Returns a function which formats a string to best represent a signal encoding a
  double precision floating point number. The function takes the *signal*,
  a real number that requires a few significant digits to accurately represent.
  The function returns a well formatted string representation of the signal.
  Throws an exception if the *signal_name* is invalid.

``signal_description()``
  Returns a string description for *signal_name*\ , if defined.

``control_description()``
  Should not be called; this ``IOGroup`` does not provide any controls.
  Throws an exception always.

``signal_behavior()``
  Returns one of the ``IOGroup::signal_behavior_e`` values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.
  Throws an exception if the *signal_name* is invalid.

``name()``
  Just calls ``plugin_name()`` under the hood.

``plugin_name()``
  Returns the name of the plugin to use when this plugin is
  registered with the ``IOGroup`` factory; see
  :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>` for more details.

``make_plugin()``
  Returns a pointer to a new ``TimeIOGroup`` object; see
  :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>` for more details.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agg(3) <geopm::Agg.3>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>`
