geopm_pio_platformcharacterization(7) -- IOGroup providing signals and controls for Platform Characterization
=============================================================================================================

Description
-----------



The PlatformCharacterization IOGroup implements the :doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`
interface to provide signals and controls used to provide per-platform characterization information.

This IOGroup provides signals that describe the energy efficient frequencies for Core,
Uncore, and GPU domains on a per node basis. This information is stored in a persistent
cached file per node, and is populated through writing the related IOGroup controls.

Requirements
^^^^^^^^^^^^

No special library support is required to use the GEOPOM PlatformCharacterization signals and controls,
however the signals will be un-initialized by default.  System administrators, researchers, or users
are expected to use the GEOPM experiment infrastructure (or other frameworks) to characterize nodes in
the system.  This information may then be written to the cache file through using the IOGroup controls.
Subsequent signal reads will then provide the values written as part of characterization for use by GEOPM
agents and users.

For basic system characterization the integrated arithimetic intensity benchmark (AIB) workload is
recommended.

The CPU Compute Activity policy recommendation script may be used to provide basic
recommendations for CPU domain characterization.

The GPU Compute Activity policy recommendation script may be used to provide basic
recommendations for GPU domain characterization.

Signals
-------

``NODE_CHARACTERIZATION::GPU_CORE_FREQUENCY_EFFICIENT``
    GPU Compute Domain energy efficient frequency in hertz.

    *  **Aggregation**: average
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: hertz

``NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT``
    CPU Core Domain energy efficient frequency in hertz.

    *  **Aggregation**: average
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: hertz

``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT``
    CPU Uncore Domain energy efficient frequency in hertz.

    *  **Aggregation**: average
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: hertz

``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_[0-14]``
    CPU Uncore frequency associated with CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_[0-14] in hertz.

    *  **Aggregation**: average
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: hertz

``NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_[0-14]``
    Maximum memory bandwidth associated with CPU_UNCORE_FREQUENCY_[0-14] in bytes/second.
    This is intended to represent the maximum bandwidth achieved at the associated frequency.

    *  **Aggregation**: average
    *  **Domain**: board
    *  **Format**: double
    *  **Unit**: bytes/second

Controls
--------

Every signal has a control of the same name.  Writing the control saves the written value to
a cached file on the node of interest.

Aliases
-------

This IOGroup provides no high-level aliases.

See Also
--------


:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3>`\ ,
:doc:`geopmwrite(1) <geopmwrite.1>`\ ,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3>`
