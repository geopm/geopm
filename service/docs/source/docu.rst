Guide for GEOPM Documentation
=============================
These are the style guidelines for writing or modifying rst source files for
GEOPM.  See :ref:`devel:Creating Manuals` section of the :doc:`Guide for GEOPM
Developers <devel>` for information on how to add the new files to the build
properly.

Some of these rules (:ref:`section ordering <docu:Section Ordering>` and the
property list in :ref:`low-level signals/controls <docu:Signals/Controls (low
level)>`) can be tested with the ``geopmlint`` Sphinx builder. Run ``make
docs_geopmlint`` to run those checks. Edit
``service/docs/source/_ext/geopmlint.py`` if you need to modify those checks.

Whitespace
----------
Minimize the amount of whitespace (carriage returns, spaces, and indentation)
present in the source file.  Only use the necessary amount of whitespace to
satisfy the rst compiler for what you are trying to do.

Sections
--------
Follow the `Sphinx Documentation for Sections
<https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html#sections>`_,
namely:

  * ``=`` for page titles
  * ``-`` for sections within a page
  * ``^`` for subsections
  * ``"`` for sub-subsections

Section Ordering
----------------
For chapter 7 man pages of IOGroups, follow the following ordering:

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
Use the following code as an example:

.. code-block:: rst

   ``PROFILE::TIME_HINT_UNKNOWN``
       The total amount of time that a CPU was measured to be running with this
       hint value on the Linux logical CPU specified.

       * **Aggregation**: average
       * **Domain**: cpu
       * **Format**: double
       * **Unit**: seconds

That block will render as follows:

``PROFILE::TIME_HINT_UNKNOWN``
    The total amount of time that a CPU was measured to be running with this
    hint value on the Linux logical CPU specified.

    * **Aggregation**: average
    * **Domain**: cpu
    * **Format**: double
    * **Unit**: seconds

MSR Signals and controls
^^^^^^^^^^^^^^^^^^^^^^^^

Document MSRs in their respective ``msr_data_*.json`` files. Use the
``geopm-msr-json`` directive to insert the formatted documentation from a json
definition into a ``rst`` file. For example:

.. code-block:: rst
   :caption: Render all MSRs in a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json

.. code-block:: rst
   :caption: Render only signals from a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json
      :no-controls:

.. code-block:: rst
   :caption: Render only controls from a json file

   .. geopm-msr-json:: ../json_data/msr_data_arch.json
      :no-signals:

If you need to modify the output format of the ``geopm-msr-json`` directive,
edit the ``GeopmMsrJson`` class in the
``service/docs/source/_ext/geopm_rst_extensions.py`` Sphinx extension.

Aliases
-------
The IOGroup specific chapter 7 pages should document how the high level
alias relates to the signals or controls provided by said IOGroup.
Use the following code as an example for 1-to-1 mappings:

.. code-block:: rst

   ``CPU_FREQUENCY_MAX_AVAIL``
       Maps to ``MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0``

That block will render as follows:

``CPU_FREQUENCY_MAX_AVAIL``
    Maps to ``MSR::TURBO_RATIO_LIMIT:MAX_RATIO_LIMIT_0``

If the alias does not directly map, then a further explanation of how the alias
is implemented is provided in the IOGroup specific page.

The :doc:`geopm_pio(7) <geopm_pio.7>` page should document the high level alias
in detail with full sentences.  Use the following code as an example:

.. code-block:: rst

   ``CPU_FREQUENCY_MAX_AVAIL``
       Maximum processor frequency.

That block will render as follows:

``CPU_FREQUENCY_MAX_AVAIL``
    Maximum processor frequency.

Examples
--------
See the following pages for examples of the style to follow:

:doc:`geopm_pio_profile(7) <geopm_pio_profile.7>`,
:doc:`geopm_pio(3) <geopm_pio.3>`,
:doc:`geopm_pio_cnl(7) <geopm_pio_cnl.7>`
