---
name: Change - New external dependency
about: Add external library, python module, device driver, sysfs feature, or command line tool
title: 'Add new external dependency: [...]'
labels: task
assignees: ''

---
> estimate 1
Parent [feature/story]: #XXXX

Done criteria:

- [ ] Use of interface exposed by external dependency is compatible
      with the BSD 3 clause license
- [ ] Shared object dependencies have been added to the build system
      in this or a prior PR
- [ ] Dependency has one or more last-level-abstraction classes if
      used in C++
- [ ] Dependency may have last-level-abstraction when used in Python
      if this enables testing
- [ ] All existing features continue to work when dependency is not
      available
- [ ] If an external python module dependency was introduced, the new
      module is added to the requirements.txt and is available via PyPI
      (pip).
- [ ] Any external package required by geopmdpy for functionality of
      the GEOPM service must be provided by RPM or debian packaging from
      standard Linux distros.
- [ ] [...]
- [ ] [...]
