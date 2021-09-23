---
name: Change - Add C++ class
about: Add a new class to the C++ implementation
title: 'Introduce new C++ class: [...]'
labels: task
assignees: ''

---
> estimate 1
Parent [feature/story]: #XXXX

Introduce new C++ class [...] with the following features:
- [...]
- [...]

Done criteria:

- [ ] Pure virtual interface class has been added with factory method
- [ ] Unit test constructor provides dependency injection
- [ ] Unit tests added for all class methods except: non-test
      constructors, private methods or classes that provide a
      last-level-abstraction of an external dependency (e.g. SDBus or
      NVMLDevicePool)
- [ ] Non-exempt methods with logic branches have multiple tests
- [ ] All dependency classes have already been merged
- [ ] All public methods have doxygen comments describing behavior,
      inputs, outputs and exceptions raised
- [ ] Implementation file has `#include "config.h"`
- [ ] [...]
- [ ] [...]
