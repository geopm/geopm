---
name: Change - Add C++ methods
about: Add new methods to an existing C++ class
title: 'Added [...] methods to [...] C++ class'
labels: task
assignees: ''

---
> estimate 1
Parent [feature/story]: #XXXX

Done criteria:

- [ ] Unit tests added for new class methods except: non-test
      constructors, private methods or classes that provide a
      last-level-abstraction of an external dependency (e.g.
      SDBus or NVMLDevicePool)
- [ ] Non-exempt methods with logic branches have multiple
      tests
- [ ] Mock objects derived from class have methods added
- [ ] New public methods have doxygen comments describing
      behavior, inputs, outputs and exceptions raised
- [ ] [...]
- [ ] [...]
