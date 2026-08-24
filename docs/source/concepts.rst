Concepts
========

The vocabulary used throughout the GEOPM documentation and command line tools.
Everything else in these guides builds on the five ideas below, so this page is
the right place to start if you have not used GEOPM before.

Signal
------

A **signal** is something GEOPM can read: a measurement or a status value
reported by the platform. Examples are ``CPU_POWER`` (Watts), ``CPU_ENERGY``
(Joules), ``CPU_FREQUENCY_STATUS`` (Hz), and ``TIME`` (seconds).

Signal names are the stable public interface to the platform. The same name
means the same thing on every system GEOPM supports, even when the underlying
mechanism differs between vendors or hardware generations.

List every signal your platform provides, with a description of each:

.. code-block:: bash

   geopmread --info-all

Control
-------

A **control** is something GEOPM can write: a hardware setting that changes how
the platform behaves. Examples are ``CPU_POWER_LIMIT_CONTROL`` (a package power
cap in Watts) and ``CPU_FREQUENCY_MAX_CONTROL`` (a maximum core frequency in
Hz).

Every control is also readable, so you can always ask what a setting currently
is before changing it. Controls are generally exposed as a signal of the same
name, which is why ``geopmread CPU_POWER_LIMIT_CONTROL package 0`` works. List
the controls available to you:

.. code-block:: bash

   geopmwrite --info-all

Not every signal has a matching control. Reading ``CPU_POWER`` tells you what
the processor is consuming; it does not imply you can set the power consumption
directly.

Domain
------

Hardware settings and measurements apply at different scopes. GEOPM calls these
scopes **domains**, and arranges them in a hierarchy: ``board`` contains
``package``, a package contains ``core``, a core contains ``cpu``, and so on.
Other domain types include ``memory``, ``gpu``, and ``nic``.

Every read and write names both a **domain type** and a **domain index**. The
index selects which instance of that domain type you mean — ``package 0`` is the
first processor socket, ``package 1`` the second.

.. code-block:: bash

   # Power for the whole board
   geopmread CPU_POWER board 0

   # Frequency of the first socket only
   geopmread CPU_FREQUENCY_STATUS package 0

Each signal and control has a **native domain**: the finest scope at which the
hardware actually implements it. Requesting a coarser domain aggregates the
underlying values, using an aggregation function appropriate to the signal
(power sums, frequency averages). Requesting a finer domain than the native one
is an error.

Two wildcards are accepted in the tools that take request lines, such as
:doc:`geopmsession(1) <geopmsession.1>`. A ``*`` in the domain-type field means
"use this signal's native domain", and a ``*`` in the domain-index field means
"every index of that domain".

.. code-block:: bash

   # Energy for every package on the system
   echo 'CPU_ENERGY package *' | geopmsession

For the full list of domain types and how they nest, see
:ref:`geopm_topo.3:Domain Types`. To inspect your own machine's topology, run
``geopmread --domain``.

Session
-------

A **session** is the lifetime of your connection to the GEOPM Access Service.
It opens when a GEOPM tool first contacts the service and closes when your
process ends.

Sessions are what make GEOPM safe for unprivileged users. The first time a
session writes a control, the service records the previous value. When the
session closes — normally, or because the process was killed, or because the
shell exited — the service **restores every control that session changed**.

This means a tuning experiment cannot permanently misconfigure the machine, and
an interrupted run leaves nothing behind to clean up. It also means a setting
you apply with :doc:`geopmwrite(1) <geopmwrite.1>` will not outlive the command
unless you keep a session open, for example with
``geopmsession --control-config``.

Access list
-----------

By default an ordinary user may read and write nothing. An administrator uses
:doc:`geopmaccess(1) <geopmaccess.1>` to grant access to specific signals and
controls, either as a system-wide default or per user and per group.

The practical consequence: a command that fails with a permission error is
usually missing an access-list entry rather than encountering a bug. Check what
you have been granted with:

.. code-block:: bash

   # Signals and controls available to you
   geopmaccess
   geopmaccess --controls

An administrator can see everything the platform is capable of, whether or not
it has been granted, by adding ``--all``.

Putting it together
-------------------

A short worked example that uses all five ideas — read a signal at a domain,
change a control, and let the session revert it:

.. code-block:: bash

   # What is the current package power cap?
   geopmread CPU_POWER_LIMIT_CONTROL package 0

   # Trace board power once per second for ten seconds
   printf 'TIME board 0\nCPU_POWER board 0\n' | geopmsession -p 1.0 -t 10.0

   # Apply a 200 W cap to package 0 for the duration of a run,
   # reverted automatically when the session ends
   echo 'CPU_POWER_LIMIT_CONTROL package 0 200' > cap.conf
   geopmsession -c cap.conf -- ./my_workload.sh

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm_topo(3) <geopm_topo.3>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmsession(1) <geopmsession.1>`,
:doc:`geopmaccess(1) <geopmaccess.1>`
