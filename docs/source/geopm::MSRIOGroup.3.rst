
geopm::MSRIOGroup -- IOGroup providing MSR-based signals and controls
=====================================================================






Synopsis
--------

#include `<MSRIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/service/src/geopm/MSRIOGroup.hpp>`_

Link with ``-lgeopmd``


.. code-block:: c++

       set<string> MSRIOGroup::signal_names(void) const override;

       set<string> MSRIOGroup::control_names(void) const override;

       bool MSRIOGroup::is_valid_signal(const string &signal_name) const override;

       bool MSRIOGroup::is_valid_control(const string &control_name) const override;

       int MSRIOGroup::signal_domain_type(const string &signal_name) const override;

       int MSRIOGroup::control_domain_type(const string &control_name) const override;

       int MSRIOGroup::push_signal(const string &signal_name,
                                   int domain_type,
                                   int domain_idx) override;

       int MSRIOGroup::push_control(const string &control_name,
                                    int domain_type,
                                    int domain_idx) override;

       void MSRIOGroup::read_batch(void) override;

       void MSRIOGroup::write_batch(void) override;

       double MSRIOGroup::sample(int sample_idx) override;

       void MSRIOGroup::adjust(int control_idx,
                               double setting) override;

       double MSRIOGroup::read_signal(const string &signal_name,
                                      int domain_type,
                                      int domain_idx) override;

       void MSRIOGroup::write_control(const string &control_name,
                                      int domain_type,
                                      int domain_idx,
                                      double setting) override;

       void MSRIOGroup::save_control(void) override;

       void MSRIOGroup::restore_control(void) override;

       function<double(const vector<double> &)> MSRIOGroup::agg_function(const string &signal_name) const override;

       function<string(double)> MSRIOGroup::format_function(const string &signal_name) const override;

       string MSRIOGroup::signal_description(const string &signal_name) const override;

       string MSRIOGroup::control_description(const string &control_name) const override;

       int MSRIOGroup::signal_behavior(const string &signal_name) const override;

       void MSRIOGroup::save_control(const string &save_path) override;

       void MSRIOGroup::restore_control(const string &save_path) override;

       string MSRIOGroup::name(void) const override;

       void MSRIOGroup::parse_json_msrs(const string &str);

       static string MSRIOGroup::msr_allowlist(int cpuid);

       static int MSRIOGroup::cpuid(void);

       static string MSRIOGroup::plugin_name(void);

       static unique_ptr<IOGroup> MSRIOGroup::make_plugin(void);

Description
-----------

The MSRIOGroup implements the :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` interface to
provide hardware signals and controls using MSRs on Intel platforms.
It relies on :doc:`geopm_pio_msr(7) <geopm_pio_msr.7>` and :doc:`geopm::MSRIO(3) <geopm::MSRIO.3>`.

Class Methods
-------------


``signal_names()``
  Returns the list of signal names provided by this ``IOGroup``.  This
  includes aliases for common hardware-based signals such as
  ``CPU_FREQUENCY_STATUS``, as well as the supported MSRs for the current platform.

``control_names()``
  Returns the list of control names provided by this ``IOGroup``.  This
  includes aliases for common hardware-based controls such as
  ``CPU_FREQUENCY_MAX_CONTROL``, as well as the supported MSRs for the current platform.

``is_valid_signal()``
  Returns whether the given *signal_name* is supported by the
  ``MSRIOGroup`` for the current platform.  Note that different
  platforms may have different supported MSRs.

``is_valid_control()``
  Returns whether the given *control_name* is supported by the
  ``MSRIOGroup`` for the current platform.  Note that different
  platforms may have different supported MSRs.

``signal_domain_type()``
  Returns the domain type for the signal specified by
  *signal_name*.  The domain for a signal may be different on
  different platforms.

``control_domain_type()``
  Returns the domain type for the control specified by
  *control_name*.  The domain for a control may be different on
  different platforms.

``push_signal()``
  Adds the signal specified by *signal_name* for *domain_type* at
  index *domain_idx* to the list of signals to be read during
  ``read_batch()``.  If the domain of a signal spans multiple Linux
  logical CPUs, only one CPU from that domain will be read, since
  all CPUs from the same domain and index will return the same
  value.

``push_control()``
  Adds the control specified by *control_name* for *domain_type* at
  index *domain_idx* to the list of controls to be written during
  ``write_batch()``.  If the domain of a control spans multiple Linux
  logical CPUs, values written to that control will be written to
  all CPUs in the domain.

``read_batch()``
  Sets up :doc:`geopm::MSRIO(3) <geopm::MSRIO.3>` for batch reading if needed, then reads
  all pushed signals through the ``MSRIO::read_batch()`` method.

``write_batch()``
  Writes all adjusted values through the :doc:`geopm::MSRIO(3) <geopm::MSRIO.3>`
  ``write_batch()`` method.

``sample()``
  Returns the value of the signal specified by a *sample_idx*
  returned from ``push_signal()``.  The value will have been updated by
  the most recent call to ``read_batch()``.

``adjust()``
  Sets the control specified by a *control_idx* returned from
  ``push_control()`` to the value *setting*.  The value will be written
  to the underlying MSR by the next call to ``write_batch()``.

``read_signal()``
  Immediately read and decode the underlying MSR supporting
  *signal_name* for *domain_type* at index *domain_idx* and return
  the result in SI units.

``write_control()``
  Immediately encode the SI unit value *setting* and write the
  correct bits of the MSR supporting *control_name* for
  *domain_type* at *domain_idx*.

``save_control()``
  Attempts to read and save the current values of all control MSRs
  for the platform.  If any control is not able to be read, it will
  be skipped.

``restore_control()``
  Using the values saved by ``save_control()``, attempts to write back
  the original values of all control MSRs.  Any control that is not
  able to be written will be skipped.

``agg_function()``
  Returns the function that should be used to aggregate
  *signal_name*.  If one was not previously specified by this class,
  the default function is ``select_first()`` from :doc:`geopm::Agg(3) <geopm::Agg.3>`.
  Throws an exception if the *signal_name* is not valid.

``format_function()``
  Returns the function that should be used to format the *signal_name*.
  If the *signal_name* string ends with a ``'#'`` character,
  the function returned is ``string_format_raw64()``.
  Throws an exception if the *signal_name* is not valid or if
  the *signal_name* was not found.

``signal_description()``
  Returns a string description for *signal_name*\ , if defined.
  Further descriptions of MSR signals can be found in
  `The Intel Software Developer's Manual <https://software.intel.com/en-us/articles/intel-sdm>`_

``control_description()``
  Returns a string description for *control_name*\ , if defined.
  Further descriptions of MSR controls can be found in
  `The Intel Software Developer's Manual <https://software.intel.com/en-us/articles/intel-sdm>`_

``signal_behavior()``
  Returns one of the ``IOGroup::signal_behavior_e`` values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

``parse_json_msrs()``
  Parse a JSON string and add any raw MSRs and
  fields as available signals and controls.

``name()``
  Calls ``plugin_name()`` internally.

``msr_allowlist()``
  Fill string with the ``msr-safe`` ``allowlist`` file contents reflecting
  all known MSRs for the current platform, or if *cpuid* is
  provided, for the platform specified by *cpuid*.  Returns a string
  formatted to be written to an ``msr-safe`` ``allowlist`` file.

``cpuid()``
  Get the ``cpuid`` of the current platform.

``plugin_name()``
  Returns the name of the plugin to use when this plugin is
  registered with the ``IOGroup`` factory; see
  :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>` for more details.

``make_plugin()``
  Returns a pointer to a new ``MSRIOGroup`` object; see
  :doc:`geopm::PluginFactory(3) <geopm::PluginFactory.3>` for more details.

Enum Type
---------

.. code-block:: c++

       enum m_cpuid_e {
           M_CPUID_SNB = 0x62D,
           M_CPUID_IVT = 0x63E,
           M_CPUID_HSX = 0x63F,
           M_CPUID_BDX = 0x64F,
           M_CPUID_KNL = 0x657,
           M_CPUID_SKX = 0x655,
           M_CPUID_ICX = 0x66A,
       };

``enum m_cpuid_e``
  Contains the list of currently-supported ``cpuid`` values.  The ``cpuid``
  can be determined by running ``lscpu`` and appending the CPU family
  in hex to the Model in hex.

Environment
-----------

If the ``GEOPM_MSR_CONFIG_PATH`` environment variable is set to a
colon-separated list of paths, the paths will be checked for files
starting with ``msr_`` and ending in ``.json``.  The ``/etc/geopm`` directory
will also be searched.  The ``MSRIOGroup`` will attempt to load additional
MSR definitions from any JSON file it finds. The files must follow this
schema:

.. literalinclude:: ../json_schemas/msrs.schema.json
    :language: json


See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopm_pio_msr(7) <geopm_pio_msr.7>`\ ,
:doc:`geopm::MSRIO(3) <geopm::MSRIO.3>`\ ,
:doc:`geopmlaunch(1) <geopmlaunch.1>`
