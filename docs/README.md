GEOPM Docs
==========
This directory contains source files and build scripts to generate GEOPM man
pages and web documentation. Subdirectories include:

* [debian](debian): Configuration files for debian packaging scripts
* [json\_data](json_data): Javascript object notation (JSON) files that are
  used generated documentation. Example: model-specific register (MSR)
  descriptions.
* [json\_schemas](json_schemas): Schemas of JSON files that are used by GEOPM
  software. These schemas may be used for validation of JSON files and may also
  be printed as part of the documentation.
* [shell\_completion](shell_completion): Scripts that generate auto-completion
  recommendations for GEOPM tools when they are used in an interactive shell.
* [source](source): ReStructured Text (rst) files used to generate man pages
  and web documentation.

Building Documentation
----------------------
Run `make man` or `make html` to build man pages or web documentation,
respectively. Generated man pages are written to the `./build/man` directory.
Generated web documentation is written to the `./build/html` directory.

Executing the Linter
--------------------
The linter (source code at
[./source/\_ext/geopmlint.py](./source/_ext/geopmlint.py))
performs the following checks:

* Documented signals should have definitions that list their properties for
  aggregation, domain, format, and unit.
* Signal properties should be described in all-lowercase text.
* `geopm_pio_*.7` docs have an expected section order: "Description",
  "Requirements", "Signals", "Controls", "Aliases", "Signal Aliases", "Control
  Aliases", and "See Also".

Packaging Documentation
-----------------------
Run `./make_deb.sh` to generate debian packages to `.deb` files in the current
directory.

Run `./make_rpm.sh` to generate rpm packages to the `$RPM_TOPDIR` directory,
or `$HOME/rpmbuild` if `$RPM_TOPDIR` is not set.
