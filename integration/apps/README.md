# Integration Application Support

For each application, there is a corresponding sub-directory with
more information on how to download and build the application.  The
supported applications are indexed in this file and a summary of the
requirements for each application directory are provided below.

Each application directory contains shell scripts that contain the required
setup, run configuration, and command line arguments for executing the
application. More information is available in the README files within
each directory.

## Application Index

#### Memory Bandwidth Bound Applications

  - [minife](minife)
  - [nekbone](nekbone)

#### Network Bandwidth Bound Applications

  - [nasft](nasft)

#### Throughput Benchmarks

  - [hpcg](hpcg)

#### Mixed Characteristics

  - [amg](amg)


## Common Code

#### `apps.py`

  Defines the `AppConf` base class which is used to launch
  applications.  Each application must implement a class derived from
  `apps.AppConf` to enable integration with the experiments.

#### `build_func.sh`

  Defines common environment variables and functions used by
  application build scripts and is typically sourced in the
  application build script.

## Application Directories

  The applications with supporting build scripts and `AppConf` classes
  are listed below in broad categories that describe the application
  characteristics.  Required files are described below.

#### `README.md`

  Each application is briefly described in the application directory
  markdown README.  Any notes or useful information about running
  experiments with the application should be noted in this file.

#### Application Python file

  In each application directory there is a python script with a name
  that matches the parent directory (e.g., `minife/minife.py`) and this
  script is responsible for defining an `apps.AppConf` derived object
  which is typically named in Pascal case after the application name
  (in our example, `MinifeAppConf`).  The `AppConf` objects are
  imported into run scripts such as in the excerpt below where the
  `app_conf` instance of the `MinifeAppConf` class is passed to the
  launch method of the monitor:
```
    from integration.apps.minife import minife
    app_conf = minife.MinifeAppConf(args.node_count)
    monitor.launch(app_conf=app_conf, args=args,
                   experiment_cli_args=extra_args)
```

#### `build.sh`

  This script downloads and builds the application, typically using functions
  defined in `build_func.sh`. This script may obtain the source by `wget`,
  `svn`, or `git`.  It may also be obtained from the local application source
  cache directory; the location of the application source cache defaults to
  `$HOME/geopm_apps` and may be overridden in the user's environment or
  `~/.geopmrc` file using the environment variable `GEOPM_APPS_SRCDIR`.

#### Source patches

  In some instances, the publicly available source code must be modified to
  enable build changes or GEOPM-related features.  These changes are encoded
  in patch files generated with the `git format-patch` command, and applied
  automatically when the `setup_source_git` `bash` function from
  `build_func.sh` is run in `build.sh`.

#### `Makefile.mk`

  All other files added for the app should be added to the `EXTRA_DIST`
  make variable in this file. A reference to this new makefile
  fragment also needs to be included by the `Makefile.mk` in the
  `apps` directory.

#### `__init__.py`

  Note that the application subdirectory must contain an `__init__.py`
  file to designate it as a python module for python 2 support.
