.. role:: raw-html-m2r(raw)
   :format: html


geopm::EnergyEfficientAgent(3) -- agent for saving energy
=========================================================






NAMESPACES
----------

The ``EnergyEfficientAgent`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::EnergyEfficientAgent``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``EnergyEfficientAgent`` class is derived from `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`_ class.

SYNOPSIS
--------

#include `<geopm/EnergyEfficientAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/EnergyEfficientAgent.hpp>`_\ 

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       void EnergyEfficientAgent::init(int level, const vector<int> &fan_in, bool is_level_root) override;

       void EnergyEfficientAgent::validate_policy(vector<double> &policy) const override;

       void EnergyEfficientAgent::split_policy(const vector<double> &in_policy,
                                               vector<vector<double> > &out_policy) override;

       bool EnergyEfficientAgent::do_send_policy(void) const override;

       void EnergyEfficientAgent::aggregate_sample(const vector<vector<double> > &in_sample,
                                                   vector<double> &out_sample) override;

       bool EnergyEfficientAgent::do_send_sample(void) const override;

       void EnergyEfficientAgent::adjust_platform(const vector<double> &in_policy) override;

       bool EnergyEfficientAgent::do_write_batch(void) const override;

       void EnergyEfficientAgent::sample_platform(vector<double> &out_sample) override;

       void EnergyEfficientAgent::wait(void) override;

       vector<pair<string, string> > EnergyEfficientAgent::report_header(void) const override;

       vector<pair<string, string> > EnergyEfficientAgent::report_host(void) const override;

       map<uint64_t, vector<pair<string, string> > > EnergyEfficientAgent::report_region(void) const override;

       vector<string> EnergyEfficientAgent::trace_names(void) const override;

       vector<function<string(double)> > EnergyEfficientAgent::trace_formats(void) const override;

       void EnergyEfficientAgent::trace_values(vector<double> &values) override;

       void EnergyEfficientAgent::enforce_policy(const vector<double> &policy) const override;

       static string EnergyEfficientAgent::plugin_name(void);

       static unique_ptr<Agent> EnergyEfficientAgent::make_plugin(void);

       static vector<string> EnergyEfficientAgent::policy_names(void);

       static vector<string> EnergyEfficientAgent::sample_names(void);

DESCRIPTION
-----------

The behavior of this agent is described in more detail in the
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_ man page.  The core of the
online algorithm is implemented in `geopm::EnergyEfficientRegion(3) <GEOPM_CXX_MAN_EnergyEfficientRegion.3.html>`_.

For more details on the implementation, see the doxygen
page at https://geopm.github.io/dox/classgeopm_1_1_energy_efficient_agent.html.

CLASS METHODS
-------------

**TODO**

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_energy_efficient(7) <geopm_agent_energy_efficient.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_\ ,
`geopm::EnergyEfficientRegion(3) <GEOPM_CXX_MAN_EnergyEfficientRegion.3.html>`_
