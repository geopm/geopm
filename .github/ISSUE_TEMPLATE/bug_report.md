---
name: Bug report
about: Create a report to help us improve
title: ''
labels: bug
assignees: ''

---
**Describe the bug**
I tried to [...] and I expected [...] instead [...]

**GEOPM version**
Output from `geopmread --version`

**Expected behavior**
A clear and concise description of what you expected to happen.

**Actual behavior**
A clear and concise description of what actually happened.

**Error messages produced**
Any output to standard error or standard output that may be related to
the bug.  Was debug messaging was enabled at build time?

_Note_:

If possible use the `--enable-debug` configuration option when
building GEOPM.  This will produce more verbose error messaging at run
time. To enable this when using the build script in the base directory
run the following command:

```bash
GEOPM_GLOBAL_CONFIG_OPTIONS="--enable-debug" ./build.sh
```

**Additional context**
Add any other context about the problem here.

_Note_:

Some helpful context could be the architecture of the system under
test, and other hardware or device driver details.  For bugs related
to the GEOPM runtime, the number of compute nodes, MPI ranks, and
OpenMP threads used by the application may be helpful context.