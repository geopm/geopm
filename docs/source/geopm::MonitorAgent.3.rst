
geopm::MonitorAgent -- agent that enforces no policies
======================================================


Namespaces
----------

The ``MonitorAgent`` class and the ``Agent`` class are members of the ``namespace geopm``\ , but
the full names, ``geopm::MonitorAgent`` and ``geopm::Agent``, have been abbreviated in this
manual.  Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::unique_ptr``\ , and ``std::function``\ , to enable better rendering of
this manual.


Synopsis
--------

#include `<geopm/MonitorAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/MonitorAgent.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       void MonitorAgent::init(int level,
                               const vector<int> &fan_in,
                               bool is_level_root) override;

       void MonitorAgent::validate_policy(vector<double> &policy) const override;

       void MonitorAgent::split_policy(const vector<double> &in_policy,
                                       vector<vector<double> > &out_policy) override;

       bool MonitorAgent::do_send_policy(void) const override;

       void MonitorAgent::aggregate_sample(const vector<vector<double> > &in_sample,
                                           vector<double> &out_sample) override;

       bool MonitorAgent::do_send_sample(void) const override;

       void MonitorAgent::adjust_platform(const vector<double> &in_policy) override;

       bool MonitorAgent::do_write_batch(void) const override;

       void MonitorAgent::sample_platform(vector<double> &out_sample) override;

       void MonitorAgent::wait(void) override;

       vector<pair<string, string> > MonitorAgent::report_header(void) const override;

       vector<pair<string, string> > MonitorAgent::report_host(void) const override;

       map<uint64_t, vector<pair<string, string> > > MonitorAgent::report_region(void) const override;

       vector<string> MonitorAgent::trace_names(void) const override;

       vector<function<string(double)> > MonitorAgent::trace_formats(void) const override;

       void MonitorAgent::trace_values(vector<double> &values) override;

       void MonitorAgent::enforce_policy(const vector<double> &policy) const override;

Description
-----------

The ``MonitorAgent`` class is a derived implementation of :doc:`geopm::Agent(3) <geopm::Agent.3>` that is used to do sampling only; no policy will be enforced.
Consequently, it inherits and overrides many of the methods of the ``Agent`` class.
These overridden methods are described in the ``Agent`` man page.
Only the methods unique to the ``MonitorAgent`` class are described here.

The behavior of this agent is described in more detail in the
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>` man page.

For more details, see the doxygen
page at https://geopm.github.io/doxall/classgeopm_1_1_monitor_agent.html.

Class Methods
-------------


``plugin_name()``
  Returns the name of the plugin.

``make_plugin()``
  Creates a new ``unique_ptr<MonitorAgent>`` and returns it.

``policy_names()``
  Returns a list of policy names.

``sample_names()``
  Returns a list of sample names.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent_monitor(7) <geopm_agent_monitor.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`
