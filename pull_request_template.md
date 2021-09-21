- Fixes #XXXX from github issues.
- [Brief description of the change]

- Required details (select one, delete others)

    + <details> <summary>Documentation fix</summary>

        - The modification was to [developer/user] documentation
        - The change was a [correction/deletion/addition]
        - The reason for the change was [...]
        - The documentation component that changed was [...]

      </details>

    + <details> <summary>Bug fix</summary>

        - [ ] Details about the bug are located in the above bug report
        - [ ] Regression test for the issue was added
        - [ ] Fix has been verified

      </details>

    + <details> <summary>New methods added to a C++ class</summary>

        - [ ] Unit tests added for new class methods except: non-test
              constructors, private methods or classes that provide a
              last-level-abstraction of an external dependency
              (e.g. SDBus or NVMLDevicePool)
        - [ ] Non-exempt methods with logic branches have multiple
              tests
        - [ ] Mock objects derived from class have methods added
        - [ ] New public methods have doxygen comments describing
              behavior, inputs, outputs and exceptions raised

      </details>

    + <details> <summary>New methods added to Python module</summary>

        - [ ] Unit test added for new public methods
        - [ ] Unit tests do not refer to private methods or members
        - [ ] New public methods have __doc__ string comments describing
              behavior, inputs, outputs and exceptions raised

      </details>

    + <details> <summary>New external dependency</summary>

        - [ ] Shared object dependencies have been added to the build
              system in this or a prior PR
        - [ ] Dependency has one or more last-level-abstraction classes
              if used in C++
        - [ ] Dependency may have last-level-abstraction when used in
              Python if this enables testing
        - [ ] All existing features continue to work when dependency is
              not available

      </details>

    + <details> <summary>A new C++ class added</summary>

        - [ ] Pure virtual interface class has been added with factory method
        - [ ] Unit test constructor provides dependency injection
        - [ ] Unit tests added for all class methods except: non-test
              constructors, private methods or classes that provide a
              last-level-abstraction of an external dependency
              (e.g. SDBus or NVMLDevicePool)
        - [ ] Non-exempt methods with logic branches have multiple
              tests
        - [ ] All dependency classes have already been merged
        - [ ] All public methods have doxygen comments describing
              behavior, inputs, outputs and exceptions raised
        - [ ] Implementation file has `#include config.h`

      </details>

    + <details> <summary>A new Python class added</summary>

        - [ ] Unit tests added for all public class methods (one test
              file per class)
        - [ ] Unit tests do not refer to private methods or members
        - [ ] Methods with logic branches have multiple tests
        - [ ] All dependency classes have already been merged
        - [ ] New public methods have __doc__ string comments describing
              behavior, inputs, outputs and exceptions raised
        - [ ] Isolate external dependencies in wrapping classes or
              modules if doing so enables better testing.

      </details>

    + <details> <summary>Removing classes or methods</summary>

        - [ ] Features have been replaced with a different implementation
        - [ ] There are no dependencies in remaining implementation, test,
              tutorials, examples or documentation
        - [ ] No regressions to existing functionality
        - [ ] Associated unit tests have been removed

      </details>

    + <details> <summary>Build system enhancement</summary>

        - [ ] The build.sh script has been updated if required
        - [ ] Changes have been documented in geopm/README.md if required
        - [ ] Changes have been documented in geopm/service/docs/build.rst if required
        - [ ] Changes have been documented in geopm/service/docs/requires.rst if required
        - [ ] RPM spec files have been updated if required

      </details>

    + <details> <summary>Closes feature request</summary>

        - [ ] Public facing documentation has been added describing
              the feature (man page, or sphinx web page update)
        - [ ] Integration test has been added that shows feature
              working as described in documentation
        - [ ] Negative tests have been added for cases where feature
              is not available
        - [ ] Documentation has been added to the service web page for
              service related features
        - [ ] New man page added for new IOGroup or Agent class

      </details>
