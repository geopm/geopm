geopm_open_pbs_power_limit(7) -- OpenPBS hook for providing job-level power limits
==================================================================================

Overview
--------

The GEOPM OpenPBS power limit hook extends OpenPBS's functionality to allow
the user to request a job-level power limit at the time of job submission. Via
a command-line option, the user can specify a per-node power limit to be
applied to every node that is part of the job. The power limit is applied just
before the job runs, as part of the job's prologue, and is restored upon job
completion, as part of the job's epilogue.

Save-restore Mechanism
----------------------

The hook will keep track of the original power limit setting and restore it at
the end of the job (for each node that's part of the job). To achieve this,
the hook needs to be configured to run during the job prologue and epilogue
events (see the Installation section for more details).

In the case where a job is interrupted unexpectedly prior to the hook being
able to restore the power limit setting, the hook will rectify the setting the
next time it is triggered on the affected nodes. For this, the hook saves the
original setting to a file prior to applying any changes. This is leveraged by
the hook during the epilogue for the restore operation and is also used to
correct the power limit setting if it was not restored as part of the previous
job. More specifically, if a new power limit is not requested in the next job
involving the affected nodes (and the save file is present), the hook will
take the opportunity to restore the power limit setting during the prologue so
the new job is not affected by a prior power limit setting.

Usage
-----

To request a power limit for a job, the user can provide the
``geopm-node-power-limit`` resource when queueing a job via ``qsub``. The
general syntax is as follows:

::

   qsub [-l geopm-node-power-limit=<power limit (watts)>]
      [<other qsub options>] [- | <script> | -- <executable> [<arguments>]]

Requirements
------------

The GEOPM power limit hook requires:

- OpenPBS
- geopmdpy (and libgeopmd) on nodes where the power limit feature is needed
- This feature requires platform vendor HW support. In particular, it requires
  HW support for the MSR::PLATFORM_POWER_LIMIT MSRs (PLATFORM_POWER_LIMIT,
  PL1_TIME_WINDOW, PL1_CLAMP_ENABLE, PL1_LIMIT_ENABLE), and the
  MSR::PLATFORM_ENERGY_STATUS MSRs.

Installation
------------

Prior to being able to use the power limit feature, the
``geopm-node-power-limit`` resource and the power limit hook need to be
installed on the OpenPBS server.

The resource can be created with the following command:

::

   qmgr -c "create resource geopm-node-power-limit type=long"

The hook is available in the ``service/integration/open_pbs`` directory
(``geopm_power_limit.py``) and it can be installed with the following
commands:

::

   qmgr -c "create hook geopm_power_limit"
   qmgr -c "import hook geopm_power_limit application/x-python default geopm_power_limit.py"
   qmgr -c "set hook geopm_power_limit event='execjob_prologue,execjob_epilogue'"

Note how the hook needs to be configured to run in two events: the job
prologue and the job epilogue.

For convenience, a script is provided to perform these installation commands:
``geopm_install_pbs_power_limit.sh``
