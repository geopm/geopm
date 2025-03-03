geopm_pio_dcgm(7) -- IOGroup providing signals and controls for NVIDIA GPUs
===========================================================================

Description
-----------

The DCGMIOGroup implements the :doc:`geopm::IOGroup(3)
<geopm::IOGroup.3>` interface to provide hardware signals for NVIDIA GPUs
from the NVIDIA Datacenter GPU Manager.  This IO Group is intended for use with
the :doc:`NVMLIOGroup <geopm_pio_nvml.7>`

Requirements
^^^^^^^^^^^^

To use the GEOPM DCGM signals and controls GEOPM must be compiled against the
NVML and DCGM libraries and must be run on a system with hardware supported by
DCGM.  To compile against the NVML and DCGM libraries GEOPM must be configured
using both the ``--enable-nvml`` flag and the ``--enable-dcgm`` flag.  The
optional flags ``--with-nvml`` and ``--with-dcgm`` may be used to indicate the
path of the required libraries.  See ``configure --help`` for more information
about these flags.

When enabling the DCGM signals, a small modification should be made to the
`geopm.service <https://github.com/geopm/geopm/blob/dev/geopmdpy/geopm.service>`_
systemd configuration file to encode the requirements on the DGCM service.  The
three lines below should be added to the ``[Unit]`` section:

.. code-block::

    [Unit]
    Wants=nvidia-dcgm.service
    After=nvidia-dcgm.service
    PartOf=nvidia-dcgm.service

The DCGM IOGroup requires an instance of nv-hostengine be running on the node
prior to loading the IOGroup.  For DCGM installation and usage information see
the `DCGM Getting Started Guide <https://docs.nvidia.com/datacenter/dcgm/latest/user-guide/getting-started.html>`_.
The DCGM IOGroup will connect to a running nv-hostengine instance if it is
available when the GEOPM service is started.  If the nv-hostengine is stopped
the geopm DCGM Signals will throw an error of the form:

.. code-block::

    "Error getting latest values for fields in read_batch: Host engine connection invalid/disconnected"

Restarting the nv-hostengine and the geopm service are required to restore
access to DCGM signals.


Signals
-------

``DCGM::SM_ACTIVE``
    Streaming Multiprocessor (SM) activity expressed as a ratio of cycles.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double.
    *  **Unit**: n/a

``DCGM::SM_OCCUPANCY``
    Warp residency expressed as a ratio of maximum warps.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

``DCGM::DRAM_ACTIVE``
    DRAM send and receive metrics expressed as a ratio of cycles.

    *  **Aggregation**: average
    *  **Domain**: gpu
    *  **Format**: double
    *  **Unit**: n/a

Controls
--------

Every control is exposed as a signal with the same name.  The relevant
signal aggregation information is provided below.

``DCGM::FIELD_UPDATE_RATE``
    Rate at which field data is polled.

    *  **Aggregation**: expect_same
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: seconds

``DCGM::MAX_STORAGE_TIME``
    The maximum time field data will be stored.

    *  **Aggregation**: expect_same
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: seconds

``DCGM::MAX_SAMPLES``
    The maximum number of samples to be stored.  Zero implies no limit.

    *  **Aggregation**: expect_same
    *  **Domain**: board
    *  **Format**: integer
    *  **Unit**: seconds

Aliases
-------

This IOGroup provides the following high-level aliases:

Signal Aliases
^^^^^^^^^^^^^^

``GPU_CORE_ACTIVITY``
    Maps to ``DCGM::SM_ACTIVE``.

``GPU_UNCORE_ACTIVITY``
    Maps to ``DCGM::DRAM_ACTIVE``.

See Also
--------


`DCGM API <https://docs.nvidia.com/datacenter/dcgm/latest/>`_\ ,
:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <geopm::Agg.3>`
:doc:`geopm_pio_nvml(7) <geopm_pio_nvml.7>`\ ,
