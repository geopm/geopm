geopm_agent(3) -- query information about available agents
============================================================

Synopsis
--------

#include `<geopm_field.h> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm_field.h>`_

Link with ``-lgeopmd``


.. code-block:: c++

       static inline uint64_t geopm_signal_to_field(double signal);

       static inline double geopm_field_to_signal(uint64_t field);


Description
-----------

The ``geopm_agent_c`` interface is used for bitwise converting
signal values returned from the ``PlatformIO``, from their 64-bit fields
into the double representation, and vice versa.


``geopm_signal_to_field()``
  Convert a *signal* that is implicitly a 64-bit field
  especially useful for converting raw 64 bit MSR fields.
  Takes in a *signal* value returned by ``PlatformIO::sample()`` or
  ``PlatformIO::read_signal()`` for a signal with a name that
  ends with the ``'#'`` character.

``geopm_signal_to_field()``
  Convert a 64-bit *field* into a double representation
  appropriate for a signal returned by an ``IOGroup``.
  Takes in an arbitrary 64-bit *field* to be stored in a
  double precision value.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_hash(3) <geopm_hash.3>`,
:doc:`geopm_pio(7) <geopm_pio.7>`
