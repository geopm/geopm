
geopm::ProfileIOGroup(3) -- IOGroup providing application signals
=================================================================


Namespaces
----------

The ``ProfileIOGroup`` class and the ``IOGroup`` class are members of the ``namespace geopm``\ , but
the full names, ``geopm::ProfileIOGroup`` and ``geopm::IOGroup``, have been abbreviated in this
manual.  Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::set``\ , ``std::unique_ptr``\ , and ``std::function``\ , to enable better rendering of
this manual.


Synopsis
--------

#include `<geopm/ProfileIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/ProfileIOGroup.hpp>`_

Link with ``-lgeopmd``


.. code-block:: c++

       set<string> ProfileIOGroup::signal_names(void) const override;

       set<string> ProfileIOGroup::control_names(void) const override;

       bool ProfileIOGroup::is_valid_signal(const string &signal_name) const override;

       bool ProfileIOGroup::is_valid_control(const string &control_name) const override;

       int ProfileIOGroup::signal_domain_type(const string &signal_name) const override;

       int ProfileIOGroup::control_domain_type(const string &control_name) const override;

       int ProfileIOGroup::push_signal(const string &signal_name,
                                       int domain_type,
                                       int domain_idx) override;

       int ProfileIOGroup::push_control(const string &control_name,
                                        int domain_type,
                                        int domain_idx) override;

       void ProfileIOGroup::read_batch(void) override;

       void ProfileIOGroup::write_batch(void) override;

       double ProfileIOGroup::sample(int signal_idx) override;

       void ProfileIOGroup::adjust(int control_idx,
                                   double setting) override;

       double ProfileIOGroup::read_signal(const string &signal_name,
                                          int domain_type,
                                          int domain_idx) override;

       void ProfileIOGroup::write_control(const string &control_name,
                                          int domain_type,
                                          int domain_idx,
                                          double setting) override;

       void ProfileIOGroup::save_control(void) override;

       void ProfileIOGroup::restore_control(void) override;

       function<double(const vector<double> &)> ProfileIOGroup::agg_function(const string &signal_name) const override;

       function<string(double)> ProfileIOGroup::format_function(const string &signal_name) const override;

       string ProfileIOGroup::signal_description(const string &signal_name) const override;

       string ProfileIOGroup::control_description(const string &control_name) const override;

       int ProfileIOGroup::signal_behavior(const string &signal_name) const override;

       void ProfileIOGroup::save_control(const string &save_path) override;

       void ProfileIOGroup::restore_control(const string &save_path) override;

       static string ProfileIOGroup::plugin_name(void);

       static unique_ptr<IOGroup> ProfileIOGroup::make_plugin(void);

       void ProfileIOGroup::connect(void);

Description
-----------

The ``ProfileIOGroup`` class is a derived implementation of :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` that provides signals from the application.
Consequently, it inherits and overrides many of the methods of the ``IOGroup`` class.
These overridden methods are described in the ``IOGroup`` man page.
Only the methods unique to the ``ProfileIOGroup`` class are described here.

For more details, see the doxygen
page at https://geopm.github.io/geopm-runtime-dox/classgeopm_1_1_profile_i_o_group.html.

Class Methods
-------------


``plugin_name()``
  Returns the name of the plugin; for ``ProfileIOGroup`` it is ``GEOPM_PROFILE_IO_GROUP_PLUGIN_NAME``,
  which expands to ``"PROFILE"``.

``make_plugin()``
  Creates a new ``unique_ptr<ProfileIOGroup>`` and returns it.

``connect()``
  Connect to the application via shared memory.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`
