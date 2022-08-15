Python Agent Tutorial
=====================
This tutorial demonstrates how to implement a python agent in geopm.
Specifically, the tutorial shows how to use an agent that can monitor
application state and can write platform settings that are applied for the
duration of an application run.

This directory includes an example implementation of a python agent that
optionally writes one or more GEOPM controls at the start of a run, and only
monitors execution for the rest of the run. Take a look at the implementation
in `static_agent.py <./static_agent.py>`_.

Monitor-only execution
----------------------
First, let's try running an application without modifying any GEOPM controls.
In this example, we are using the stress-ng tool to stress 20 CPUs for 5 seconds.
We'll run stress-ng with ``--taskset 0-19`` so we know which CPUs to expect will
run ``stress-ng``. This isn't strictly necessary, and is only being done to help
us know where to expect most of our CPU load when walking through this
tutorial.

.. code-block:: sh

    ./static_agent.py --geopm-report stress-monitor.report -- \
        stress-ng --cpu 20 --taskset 0-19 --timeout 5

The resulting ``stress-monitor.report`` file is a YAML-formatted file containing the inputs
to your agent and the observed GEOPM signals over your run. For example:

.. raw:: html

        <details><summary>Report file contents</summary><p>

.. code:: yaml

    GEOPM Version: 2.0.0~rc2+dev112g5c3c4597a
    Start Time: Thu Aug 11 06:03:32 2022
    Profile: stress-ng --cpu 20 --taskset 0-19 --timeout 5
    Agent: StaticAgent
    Policy:
      Initial Controls: {}
    Hosts:
      tutorial-hostname:
        Application Totals:
          runtime (s): 5.00064
          count: 0
          sync-runtime (s): 5.00064
          package-energy (J): 881.021
          dram-energy (J): 60.7762
          power (W): 176.182
          frequency (%): 135.539
          frequency (Hz): 2846320000.0
          uncore-frequency (Hz): 2350000000.0
          geopmctl memory HWM (B): 32380
          geopmctl network BW (B/s): 0

.. raw:: html

        </p></details>

Let's try looking at a per-package breakdown of the energy consumption. We'll
expect that CPU 0 is on package 0 and should show more energy consumption since
that's where we are running stress-ng.

.. code-block:: sh

    ./static_agent.py --geopm-report stress-package-energy.report \
        --geopm-report-signals CPU_ENERGY@package -- \
        stress-ng --cpu 20 --taskset 0-19 --timeout 5

As expected, we can see that package 0 (running stress-ng) consumed more
energy than package 1.

.. raw:: html

        <details><summary>Report file contents</summary><p>

.. code:: yaml

    GEOPM Version: 2.0.0~rc2+dev112g5c3c4597a
    Start Time: Thu Aug 11 06:03:40 2022
    Profile: stress-ng --cpu 20 --taskset 0-19 --timeout 5
    Agent: StaticAgent
    Policy:
      Initial Controls: {}
    Hosts:
      tutorial-hostname:
        Application Totals:
          runtime (s): 5.00062
          count: 0
          sync-runtime (s): 5.00062
          package-energy (J): 885.778
          dram-energy (J): 61.3
          power (W): 177.134
          frequency (%): 135.091
          frequency (Hz): 2836900000.0
          uncore-frequency (Hz): 2350000000.0
          CPU_ENERGY@package-0: 635.942
          CPU_ENERGY@package-1: 249.837
          geopmctl memory HWM (B): 32648
          geopmctl network BW (B/s): 0

.. raw:: html

        </p></details>

Limiting CPU Frequency During Execution
---------------------------------------
Now let's try applying a limit to the frequency on all CPU cores during the
stress run. But before we do that, use ``geopmread`` to take note of the current
frequency control value. We'll refer back to this value later.

.. code-block:: sh

    geopmread CPU_FREQUENCY_MAX_CONTROL board 0

.. code-block:: sh

    ./static_agent.py --geopm-report stress-frequency-limit.report \
       --geopm-report-signals CPU_ENERGY@package \
       --geopm-initialize-control CPU_FREQUENCY_MAX_CONTROL=1.5e9 -- \
       stress-ng --cpu 20 --taskset 0-19 --timeout 5

Notice a few changes in the report. First, we can see that the Policy section
has been updated to indicate that a frequency control was applied by the agent,
as requested by the agent's user. If you plan to implement your own agent
that has user-configurable inputs, consider adding them to the Policy output in
your agent's report. See how this is done in the agent's ``get_report()``
function.

Next, we can see that the requested frequency change actually impacted the
average frequency that the CPU cores achieved in this run. This is visible
as ``frequency (Hz)`` in the report. The ``CPU_ENERGY`` measurements decreased
as a result of lowering the CPU frequency without impacting run time (since we
configured stress-ng to run for 5 seconds regardless of how much work was
actually done).

.. raw:: html

        <details><summary>Report file contents</summary><p>

.. code:: yaml

    GEOPM Version: 2.0.0~rc2+dev112g5c3c4597a
    Start Time: Thu Aug 11 06:03:48 2022
    Profile: stress-ng --cpu 20 --taskset 0-19 --timeout 5
    Agent: StaticAgent
    Policy:
      Initial Controls:
        CPU_FREQUENCY_MAX_CONTROL: 1500000000.0
    Hosts:
      tutorial-hostname:
        Application Totals:
          runtime (s): 5.00074
          count: 0
          sync-runtime (s): 5.00074
          package-energy (J): 459.749
          dram-energy (J): 61.3112
          power (W): 91.9362
          frequency (%): 71.4218
          frequency (Hz): 1499860000.0
          uncore-frequency (Hz): 1200000000.0
          CPU_ENERGY@package-0: 294.2
          CPU_ENERGY@package-1: 165.549
          geopmctl memory HWM (B): 32820
          geopmctl network BW (B/s): 0

.. raw:: html

        </p></details>

Lastly, use ``geopmread`` to take a look at the current CPU frequency control.
Notice that the current CPU frequency limit is the same as before our
experiment, even though the application ran under a different limit! If you
search through the agent's source code, you will not find any code that saves
and restores this setting. The save/restore functionality comes from the
GEOPM runtime that invokes your agent's code. If you implement your own
agent, be sure that you do not modify GEOPM controls before your agent's
``run_begin()`` function is called or after your ``run_end()`` is called. Or if
you must, then know that you will have to implement your own save/restore code.

Now you have an example implementation of a GEOPM python agent, and some
expectations about how you can use an agent and its output. Try implementing
your own agent!
