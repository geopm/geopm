GEOPM Documentation Guidelines
==============================

This guide outlines the style specifications for writing or amending
reStructuredText (rst) source files for the GEOPM project. Please refer to the
:ref:`devel:Creating Manuals` section in the :doc:`Guide for GEOPM Developers
<devel>` for detailed instructions on properly adding new files to the build.

Certain rules such as :ref:`section ordering <docu:Section Ordering>` and the
property list mentioned in :ref:`low-level signals/controls
<docu:Signals/Controls (low level)>`, can be verified using the ``geopmlint``
Sphinx builder. Execute ``make docs_geopmlint`` command to perform these checks.
If it's necessary to alter these checks, modify
``service/docs/source/_ext/geopmlint.py``.

Whitespace
----------

Strive to minimize the usage of whitespace (carriage returns, spaces, and
indentation) in the source file. Use only the necessary amount of whitespace
as required for readability or by the reStructuredText compiler for your goal.

Columns in a Line
-----------------

The number of columns in a source file should not exceed 70 or 80 before
wrapping the line.  Exceptions are allowed when it is required for compilation
or similar.  In general, follow the style in the file you are modifying.

Sections
--------

Adhere to the `Sphinx Documentation for Sections
<https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#sections>`,
explicitly:

  * ``=`` for page titles
  * ``-`` for sections within a page
  * ``^`` for subsections
  * ``"`` for sub-subsections

Section Ordering
----------------
For Chapter 7 man pages related to IOGroups, follow the following sequence:

1. Description
2. Requirements
3. Signals
4. Controls
5. Aliases

   a. Signal Aliases
   b. Control Aliases

6. See Also

Signals/Controls (low level)
----------------------------
Use the following code as an illustration:

.. code-block:: rst

   ``PROFILE::TIME_HINT_UNKNOWN``
       The aggregate time that a CPU was monitored while working with this
       specific hint value on the designated Linux logical CPU.

       * **Aggregation**: average
       * **Domain**: cpu
       * **Format**: double
       * **Unit**: seconds

This will appear as:

``PROFILE::TIME_HINT_UNKNOWN``
    The aggregate time that a CPU was monitored while working with this
    specific hint value on the designated Linux logical CPU.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

MSR Signals and controls
^^^^^^^^^^^^^^^^^^^^^^^^

MSRs should be documented in their related ``msr_data_*.json`` file. Apply the
``geopm-msr-json`` directive to extract the formatted documentation from a JSON
definition into a ``rst`` file. See the examples below:

.. code-block:: rst
   :caption: All MSRs in a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json

.. code-block:: rst
   :caption: Only signals from a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json
      :no-controls:

.. code-block:: rst
   :caption: Only controls from a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json
      :no-signals:

To change the output format of the ``geopm-msr-json`` directive,
revise the ``GeopmMsrJson`` class in the
``service/docs/source/_ext/geopm_rst_extensions.py`` Sphinx extension.

Aliases
-------
The Chapter 7 pages particular to an IOGroup should describe how the high-level
alias corresponds to the signals or controls provided by that IOGroup.
This is an example of a direct 1-to-1 mapping:

.. code-block:: rst

   ``CPU_FREQUENCY_MAX_AVAIL``
       Corresponds to ``MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0``

It will display as:

``CPU_FREQUENCY_MAX_AVAIL``
    Corresponds to ``MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0``

If the alias does not have a straightforward mapping, then an expanded
explanation of how the alias is implemented should be provided in the IOGroup
specific page.

The :doc:`geopm_pio(7) <geopm_pio.7>` page should explain the high-level alias
in full sentence descriptions. Here's an example:

.. code-block:: rst

   ``CPU_FREQUENCY_MAX_AVAIL``
       Maximum available processor frequency.

It will render as:

``CPU_FREQUENCY_MAX_AVAIL``
    Maximum available processor frequency.

Examples
--------
For style reference, consult the following pages:

:doc:`geopm_pio_profile(7) <geopm_pio_profile.7>`,
:doc:`geopm_pio(3) <geopm_pio.3>`,
:doc:`geopm_pio_cnl(7) <geopm_pio_cnl.7>`
