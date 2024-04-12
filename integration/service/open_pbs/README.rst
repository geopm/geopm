geopm_open_pbs_power_limit(7) -- OpenPBS hooks for providing job-level power limits
===================================================================================

Overview
--------

The GEOPM OpenPBS power limit hook extends OpenPBS's functionality to allow
the user to request a job-level power limit through a ``geopm-job-power-limit``
resource at the time of job submission and to enforce cluster-level power
limits through that mechanism. Via a command-line option, the user can specify
a per-node power limit through a ``geopm-node-power-limit`` resource to be
applied to every node that is part of the job. The power limit is applied just
before the job runs as part of the job's prologue, and it is restored upon job
completion as part of the job's epilogue. The scheduler can be configured
with constraints over those power limits across a cluster in order to enforce a
cluster power limit. The GEOPM OpenPBS hook enables automatic configuration of
job power limits to increase utilization of the available cluster power.

.. image:: https://geopm.github.io/images/geopm_power_resources.svg
     :width: 600
     :alt: Block diagram of the relationship between geopm node and job power resources

Save-restore Mechanism
----------------------
The hook keeps track of the original power limit setting and restores it at
the end of the job (for each node that's part of the job). To achieve this,
the hook needs to be configured to run during the job prologue and epilogue
events (see the Installation section for more details).

The hook saves the original power limit to a file prior to applying any new
power limits. The job's epilogue hook leverages the save file for the restore
operation and in the prologue if it was not restored as part of the previous
job. The save file is removed when the power limits are restored (regardless of
whether the prologue or epilogue handles the restore operation).

If a job is interrupted unexpectedly prior to restoring the power limit
setting, the hook will rectify the power setting the next time the hook
executes on the affected nodes. More specifically, if a new power limit is not
requested in the next job involving the affected nodes (and the save file is
present), the hook will restore the power limit setting during the prologue so
the new job is not affected by a prior power limit setting.

Automatic Power Limits
----------------------
The goal of the automatic power limiter is to provide additional scheduling
flexibility when the cluster is power constrained. If the cluster power cap is
sufficiently low, then jobs may not begin executing even when sufficient
non-power resources are available on idle nodes. At a high level,
this automatic limiter attempts to set the lowest acceptable power cap to
each job, based on modeled power-performance relationships and a desired
slowdown limit.

Automatic power limits are selected through a handler for ``queuejob`` PBS
events. The ``queuejob`` event handler initially sets the
``geopm-job-power-limit`` of each submitted job to be the lowest tolerable
power limit for that job, according to the configured maximum job slowdown and
the job's selected power-performance model. If the user explicitly requests a
``geopm-job-power-limit`` or a ``geopm-node-power-limit`` on job submission,
then automatic power limit selection is not applied.

If both the ``geopm-job-power-limit`` and the ``geopm-node-power-limit`` are
requested by a user, then the ``queuejob`` hook sets a
``geopm-job-power-limit`` limit that is equivalent to the greater of the two,
to conservatively ensure that the user receives at least the amount of
requested power under either interpretation. For example, in a 2-node job with
a 200 W node power limit and a 200 W job power limit, the job is assigned a 200
W node power limit and a 400 W job power limit.

Usage
-----
This section describes user interfaces for end users of the cluster to specify
power limits for their jobs and for system administrators to configure cluster
power limits.

Setting Node Power Limits For a Job
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To request a uniform power limit across all nodes in a job, a user can
provide the ``geopm-node-power-limit`` resource when queueing a job via
``qsub``. The general syntax is as follows:

::

   qsub [-l geopm-node-power-limit=<power limit (watts)>]
      [<other qsub options>] [- | <script> | -- <executable> [<arguments>]]

Enforcing a Cluster Power Limit Through Job Power Limits
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To request a power limit for the cluster, an administrator can set the total
watts of node power available across reservations and running jobs associated
with the PBS server. The syntax is as follows:

::

    qmgr -c 'set server resources_available.geopm-job-power-limit=<power limit (watts)>'

This tells PBS not to start executing a job if the job's assigned power limit
plus the power limits of other overlapping jobs and reservations in the
schedule would exceed the assigned cluster power limit.

The ``resources_available`` limit *does not* account for any resources that are
not tracked in PBS or that are used outside of the context of a running job.
For example, idle power consumption is not automatically accounted by this
mechanism.

If no power limit is specified for a job or for its nodes, then the job will
default to requesting the maximum allowed power. To set the maximum allowed
power per node:

::

    qmgr -c 'set server resources_available.geopm-max-node-power-limit=<power per node (W)>'

If a cluster power cap is set, then a queued job may be delayed due to waiting
for a period with sufficient power in the PBS server's available resources. The
chance of delay may be reduced by requesting a lower power cap for the job. A
user can limit the maximum tolerable slowdown by specifying the slowdown factor
as ``-l geopm-max-slowdown=<slowdown>`` in the ``qsub`` command.  A value of
zero indicates a preference to assign full power to the job. A value of 1.0
indicates tolerance for 100% slowdown (2x execution time). Higher tolerances
for slowdown may result in less time waiting in the job queue if the cluster is
power-limited, since the submitted job may request less power capacity as a
result. The job's power cap is computed from the slowdown based on the
performance model for the selected job type, specified by ``-l
geopm-job-type=JOB_TYPE_NAME``. See the :ref:`Configuring Performance Models`
section for information about how to define the performance models.

The automatically assigned power limit will never be less than
``geopm-min-node-power-limit`` watts per node. To set the minimum automatically
assignable power per node:

::

    qmgr -c 'set server resources_available.geopm-min-node-power-limit=<power per node (W)>'

Configuring Performance Models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
By default, the automatic power capper relates power caps to slowdown by
assuming a direct mapping between power and slowdown (i.e., 50% of max power
results in 50% slowdown).

An administrator can also configure polynomial power-performance models for
individual *job classes* or for *compute nodes*. In both cases, the models are
configured in terms of coefficients in the following relationship between $S$
slowdown and $P$ power cap:

$S(P) = (A (x0 - P / P_{max})^2 + B (x0 - P / P_{max}) + C)$

Each performance model maps to a *profile* name, optionally also to one or
more *host* names within the profile. Models that have a profile but not a host
are used to relate user-requested slowdown to job power caps. Models that have
both a profile and a host are used to non-uniformly distribute a job power cap
across nodes assigned to the job.

Each host or profile model is defined in a *model* attribute that contains
members for the model coefficients ``x0``, ``A``, ``B``, and ``C``. The root
level of the configuration file also has a ``max_power`` value that maps to
$P_{max}$. The following example configuration file defines coefficients for
job classes named ``jobtype_a`` and ``jobtype_b``, and defines per-host models
under a profile named ``node-characterization``:

.. code-block::
    :caption: Contents of an example configuration file, geopm_pbs_config.json

    {
      "node_profile_name": "node-characterization",
      "max_power": 3700.0,
      "profiles": {
        "jobtype_a": {
          "model": {
            "x0": 1.0,
            "A": 8e-08,
            "B": 7e-05,
            "C": 1.0
          }
        },
        "jobtype_b": {
          "model": {
            "x0": 1.0,
            "A": 8e-08,
            "B": 2e-06,
            "C": 1.0
          }
        },
        "node-characterization": {
          "hosts": {
            "host1": {
              "model": {
                "x0": 1.0,
                "A": 8e-08,
                "B": 2e-06,
                "C": 1.0
              }
            }
          }
        }
      }
    }

Configuration files can be validated against the schema at
``geopm_pbs_hook_config.schema.json``.

This directory contains a script (``generate_coefficients_from_reports.py``)
that generates coefficients to include in the configuration file, given a
collection of GEOPM reports from running profiled job types under varying node
power caps.

Example usage to generate job performance model coefficients:

::

    ./generate_coefficients_from_reports.py <max power per node> --reports /path/to/power/sweep/*.report

Example usage to generate compute-node performance model coefficients:

::

    ./generate_coefficients_from_reports.py <max power per node> --per-host --reports /path/to/power/sweep/*.report

Import a configuration file into PBS via the following ``qmgr`` command:

::

    qmgr -c 'import hook geopm_power_limit application/x-config default geopm_pbs_config.json'

Requirements
------------

The GEOPM power limit hooks require:

- OpenPBS
- geopmdpy (and libgeopmd) on nodes where the power limit feature is needed
- This feature requires platform vendor HW support. In particular, it requires
  HW support for the ``MSR::PLATFORM_POWER_LIMIT`` MSRs (``PL1_POWER_LIMIT``,
  ``PL1_TIME_WINDOW``, ``PL1_CLAMP_ENABLE``, ``PL1_LIMIT_ENABLE``), and the
  ``MSR::PLATFORM_ENERGY_STATUS`` MSRs.

Installation
------------
Prior to being able to use the power limit feature, the
``geopm-node-power-limit`` and ``geopm-job-power-limit`` resources and the
power limit hook need to be installed on the OpenPBS server.

The resources can be created with the following commands:

::

   qmgr -c "create resource geopm-node-power-limit type=float"
   qmgr -c "create resource geopm-min-node-power-limit type=float,flag=r"
   qmgr -c "create resource geopm-max-node-power-limit type=float,flag=r"
   qmgr -c "create resource geopm-job-power-limit type=float,flag=q"
   qmgr -c "create resource geopm-max-slowdown type=float"
   qmgr -c "create resource geopm-job-type type=string"

To ensure that the job power limit is treated as a consumable resource by the
PBS scheduler, the resource must also be declared in the scheduler's
configuration file (located at ``$PBS_HOME/sched_priv/sched_config``). That is,
the line beginning with ``resources:`` should include an entry for
``geopm-job-power-limit``. The other resources mentioned in this guide do not
need to be added to that configuration file. For example, the following
commands add the GEOPM resource to an existing scheduler configuration:

::

    source /etc/pbs.conf
    sed -i -e 's/^resources: "\([^"]\+\)"$/resources: "\1, geopm-job-power-limit"/g' "${PBS_HOME}/sched_priv/sched_config"

Restart the PBS scheduler for the configuration changes to take effect. For
example, one way is to use systemctl or run ``/etc/init.d/pbs restart`` where
the PBS scheduler is running.

The hook to enforce node power caps is available in the GEOPM
``integration/service/open_pbs`` directory (``geopm_power_limit.py``) and
can be installed with the following commands:

::

   qmgr -c "create hook geopm_power_limit"
   qmgr -c "import hook geopm_power_limit application/x-python default geopm_power_limit.py"
   qmgr -c "set hook geopm_power_limit event='execjob_prologue,execjob_epilogue,queuejob'"

Note how the hook needs to be configured to run in three events: job prologue,
job epilogue, queuejob. The purpose of each hook is illustrated in the job
submission timeline below.

.. code:: mermaid

  flowchart TD
    submit["User submits job"]
    queuejob["<b>queuejob hook</b>
      Set job's power to minimum power that meets slowdown requirements"]
    start["Job Starts"]
    prologue["<b>execjob_prologue hook</b>
      Save current node power limits and set new limits equal to the job's allocated power per node"]
    finish["Job Finishes"]
    epilogue["<b>execjob_epilogue hook</b>
      Restore saved power limits."]

    submit:::event-->queuejob:::hook-->start:::event-->prologue:::hook-->finish:::event-->epilogue:::hook
    classDef event fill:#dde9af
    classDef hook fill:#4472c4,color:#fff

For convenience, a script is provided to perform these installation commands:
``geopm_install_pbs_power_limit.sh``
