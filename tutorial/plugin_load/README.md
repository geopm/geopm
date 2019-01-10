This tutorial demonstrates loading multiple IOGroup plugins in
different orders, which affects which implementation of signals and
controls will be used by GEOPM. There are two subdirectories, alice
and bob, each containing a single IOGroup plugin.  These IOGroups do
nothing except provide descriptions for several signal names.  For a
more complete example of how to build a plugin for GEOPM, please refer
to the iogroup tutorial found in tutorial/iogroup.

To start, observe the default list of signals with their descriptions
available on your system using `geopmread --info`.

    $ geopmread --info
    CPUINFO::FREQ_MAX:
    CPUINFO::FREQ_MIN:
    ...
    POWER_DRAM: Average DRAM power in watts over the last 8 samples (usually 40 ms).
    POWER_PACKAGE: Average package power in watts over the last 8 samples (usually 40 ms).
    TIME: Time in seconds since the IOGroup load.
    TIME::ELAPSED: Time in seconds since the IOGroup load.
    TIMESTAMP_COUNTER:
    ...

Note that there is a built-in signal called "TIME" that provides time
in seconds.  By default, this alias is supported by the same IOGroup
that provides "TIME::ELAPSED".

Now try adding one of the two subdirectories to the GEOPM_PLUGIN_PATH:

    $ GEOPM_PLUGIN_PATH=$PWD/alice geopmread --info
    BAR: Alice's bar signal
    CPUINFO::FREQ_MAX:
    ...
    FOO: Alice's foo signal
    ...
    TIME: Alice's time signal
    TIME::ELAPSED: Time in seconds since the IOGroup load.
    TIMESTAMP_COUNTER:
    ...

The new IOGroup's signals have been added to the list.  In addition,
the new IOGroup plugin also provides an implementation of the "TIME"
signal, which will be used in place of the default.

When both subdirectories are added to the GEOPM_PLUGIN_PATH, their
order matters.  The two IOGroup plugins in this example overload some
of the same signal names.  If the path to "alice" is first in the
list, the versions of the signals from the "alice" plugin will be used
for "BAR" and "TIME".  Since the "alice" plugin does not provide a
"BAZ" signal, the signal from the "bob" plugin will be used.

    $ GEOPM_PLUGIN_PATH=$PWD/alice:$PWD/bob geopmread --info
    BAR: Alice's bar signal
    BAZ: Baz signal from Bob
    CPUINFO::FREQ_MAX:
    CPUINFO::FREQ_MIN:
    ...
    FOO: Alice's foo signal
    ...
    TIME: Alice's time signal
    TIME::ELAPSED: Time in seconds since the IOGroup load.
    TIMESTAMP_COUNTER:
    ...

Likewise, if 'bob' is first in the list, the versions of the signals from
the bob plugin will be used for "BAR" and "TIME".
Since the "FOO" signal is not provided by the
"bob" plugin, its implementation is still provided by the "alice" plugin.

    $ GEOPM_PLUGIN_PATH=$PWD/bob:$PWD/alice geopmread --info
    BAR: Bar signal from Bob
    BAZ: Baz signal from Bob
    CPUINFO::FREQ_MAX:
    CPUINFO::FREQ_MIN:
    ...
    FOO: Alice's foo signal
    ...
    TIME: Time signal from Bob
    TIME::ELAPSED: Time in seconds since the IOGroup load.
    TIMESTAMP_COUNTER:
    ...
