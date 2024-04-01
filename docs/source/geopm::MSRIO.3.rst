
geopm::MSRIO(3) -- methods for reading and writing MSRs
=======================================================


Namespaces
----------

The ``MSRIO`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::MSRIO``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::set``\ , ``std::unique_ptr``\ , ``std::shared_ptr``
to enable better rendering of this manual.

Synopsis
--------

#include `<geopm/MSRIO.hpp> <https://github.com/geopm/geopm/blob/dev/service/src/MSRIO.hpp>`_

Link with ``-lgeopmd``


.. code-block:: c++

       virtual uint64_t MSRIO::read_msr(int cpu_idx,
                                        uint64_t offset) = 0;

       virtual void MSRIO::write_msr(int cpu_idx,
                                     uint64_t offset,
                                     uint64_t raw_value,
                                     uint64_t write_mask) = 0;

       virtual int MSRIO::add_read(int cpu_idx, uint64_t offset) = 0;

       virtual void MSRIO::read_batch(void) = 0;

       virtual int MSRIO::add_write(int cpu_idx, uint64_t offset) = 0;

       virtual void MSRIO::adjust(int batch_idx,
                                  uint64_t value,
                                  uint64_t write_mask) = 0;

       virtual uint64_t MSRIO::sample(int batch_idx) const = 0;

       virtual void MSRIO::write_batch(void) = 0;

       static unique_ptr<MSRIO> MSRIO::make_unique(void);

       static shared_ptr<MSRIO> MSRIO::make_shared(void);

Description
-----------

The MSRIO class handles reading and writing to Model-Specific Registers (MSRs).
The implementation uses msr-safe, found at https://github.com/LLNL/msr-safe
to allow access to a controlled set of MSRs from user space.
Refer to :doc:`geopm_pio_msr(7) <geopm_pio_msr.7>` for more details.

This class is an abstract base class.

Class Methods
-------------


``read_msr()``
  Read from a single MSR at *offset* on the logical Linux CPU
  specified by *cpu_idx*.  Returns the raw encoded MSR value.

``write_msr()``
  Write to a single MSR at *offset* on the logical Linux CPU
  specified by *cpu_idx*.  The value in *raw_value* will be masked
  with *write_mask* and the value will only be written for bits
  where the *write_mask* is ``1``.  All other bits in the MSR will remain
  unmodified.  If the *raw_value* tries to write any bits outside
  the mask, an error will be raised.

``add_read()``
  Add an *offset* to the list of MSRs to be read by the next call to
  ``read_batch()``, extend this set of MSRs with a single *offset*.
  The *cpu_idx* is the logical Linux CPU index to read from when
  ``read_batch()`` method is called.
  Returns the logical index that will be passed to ``sample()``.

``read_batch()``
  Batch read a set of MSRs configured by a previous call to the
  ``batch_config()`` method.  The memory used to store the result should have
  been returned by ``add_read()``.  The resulting raw encoded MSR values are
  accessible through ``sample()``.

``add_write()``
  Add another *offset* to the list of MSRs to be written in batch.
  The *cpu_idx* is the logical Linux CPU index to write to when
  ``write_batch()`` method is called.
  Returns the logical index that will be passed to ``adjust()``.

``adjust()``
  Adjust a *value* that was previously added with the ``add_write()`` method.
  The *value* in will be masked with *write_mask* and the *value*
  will only be written for bits where the *write_mask* is ``1``.
  All other bits in the MSR will remain unmodified.
  If the *value* tries to write any bits outside the mask, an error will be raised.

``sample()``
  Read the full 64-bit value of the MSR that was previously added
  to the MSRIO batch with ``add_read()``.  ``read_batch()`` must be called
  prior to calling ``sample()``.

``write_batch()``
  Batch write a set of MSRs configured by a previous call to the
  ``batch_config()`` method.  The values in the *raw_value* vector will
  be written to the corresponding configured locations.

``make_unique()``
  Returns a ``unique_ptr`` to a concrete object constructed using the underlying implementation

``make_shared()``
  Returns a ``shared_ptr`` to a concrete object constructed using the underlying implementation

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_pio_msr(7) <geopm_pio_msr.7>`\ ,
:doc:`geopm::MSRIOGroup(3) <geopm::MSRIOGroup.3>`
