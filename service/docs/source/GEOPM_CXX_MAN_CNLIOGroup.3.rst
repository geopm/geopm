.. role:: raw-html-m2r(raw)
   :format: html


geopm::CNLIOGroup(3) -- IOGroup for interaction with Compute Node Linux
=======================================================================






NAMESPACES
----------

The ``CNLIOGroup`` class and the ``IOGroup`` class are members of
the ``namespace geopm``, but the full names, ``geopm::CNLIOGroup`` and
``geopm::IOGroup``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

SYNOPSIS
--------

#include `<geopm/CNLIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/src/CNLIOGroup.hpp>`_\ 

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       set<string> CNLIOGroup::signal_names(void) const override;

       set<string> CNLIOGroup::control_names(void) const override;

       bool CNLIOGroup::is_valid_signal(const string &signal_name) const override;

       bool CNLIOGroup::is_valid_control(const string &control_name) const override;

       int CNLIOGroup::signal_domain_type(const string &signal_name) const override;

       int CNLIOGroup::control_domain_type(const string &control_name) const override;

       int CNLIOGroup::push_signal(const string &signal_name, int domain_type,
                                   int domain_idx) override;

       int CNLIOGroup::push_control(const string &control_name, int domain_type,
                                    int domain_idx) override;

       void CNLIOGroup::read_batch(void) override;

       void CNLIOGroup::write_batch(void) override;

       double CNLIOGroup::sample(int batch_idx) override;

       void CNLIOGroup::adjust(int batch_idx, double setting) override;

       double CNLIOGroup::read_signal(const string &signal_name, int domain_type,
                                      int domain_idx) override;

       void CNLIOGroup::write_control(const string &control_name, int domain_type,
                                      int domain_idx, double setting) override;

       void CNLIOGroup::save_control(void) override;

       void CNLIOGroup::restore_control(void) override;

       function<double(const vector<double> &)>
           CNLIOGroup::agg_function(const string &signal_name) const override;

       function<string(double)>
           CNLIOGroup::format_function(const string &signal_name) const override;

       string CNLIOGroup::signal_description(const string &signal_name) const override;

       string CNLIOGroup::control_description(const string &control_name) const override;

       int CNLIOGroup::signal_behavior(const string &signal_name) const override;

       void CNLIOGroup::save_control(const string &save_path) override;

       void CNLIOGroup::restore_control(const string &save_path) override;

       static string CNLIOGroup::plugin_name(void);

       static unique_ptr<IOGroup> CNLIOGroup::make_plugin(void);

DESCRIPTION
-----------

The CNLIOGroup provides board-level energy counters from Compute Node Linux
as signals.  These values are obtained through the `proc(5) <http://man7.org/linux/man-pages/man5/proc.5.html>`_ filesystem.

CLASS METHODS
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
  CNLIOGroup for the current platform.

* 
  ``is_valid_control()``:
  Returns false; this IOGroup does not provide any controls.

* 
  ``signal_domain_type()``:
  If the *signal_name* is valid for this IOGroup, returns ``GEOPM_DOMAIN_BOARD``.
  Otherwise, returns ``GEOPM_DOMAIN_INVALID``.

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
  Read all pushed signals from the platform so that the next call to
  ``sample()`` will reflect the updated data.  The intention is that
  ``read_batch()`` will read the all of the ``IOGroup``\ 's signals into memory once
  per call.

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
  Return a function that should be used when aggregating the given
  signal.  For more information see `geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_.

* 
  ``format_function()``:
  Return a function that should be used when formatting the given
  signal.  For more information see `geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_.

* 
  ``signal_description()``:
  Returns a string description for *signal_name*, if defined.

* 
  ``control_description()``:
  Should not be called; this IOGroup does not provide any controls.
  throws an error

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
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

* 
  ``make_plugin()``:
  Returns a pointer to a new CNLIOGroup object; see
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`proc(5) <http://man7.org/linux/man-pages/man5/proc.5.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_
