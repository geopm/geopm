Shell Completion Scripts
========================
This directory contains scripts to enable shell completion for GEOPM tools.

The non-extension part of each file name indicates which tool's interface is
autocompleted by the script. The extension part of the file name indicates the
target completion environment. For example, geopmlaunch.bash_completion defines
a Bash autocompletion script for geopmlaunch.

Typically, these scripts can be enabled by sourcing them into your shell
environment or by installing them to a location where they will be automatically
sourced. For example, you can add `source /path/to/geopmlaunch.bash_completion`
to your `.bashrc`.
