#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import glob
import sphinx_rtd_theme

sys.path.insert(0, os.path.abspath('../..'))


# -- Project information -----------------------------------------------------

project = 'GEOPM'
copyright = '2015 - 2022, Intel Corporation. All rights reserved.'

author = 'Intel (R) Corporation'

# -- Helper functions --------------------------------------------------------

# gets the description of the man page from the rst file
def get_description(rst_file_name):
    previous_line = ""
    current_line = ""
    with open(rst_file_name, 'r') as rst_file:
        # loop through each line of text
        for line in rst_file:
            line = line.rstrip()
            previous_line = current_line
            current_line = line
            # if you find the starting header
            if (current_line and current_line == len(current_line) * '='):
                rst_file.close()
                return previous_line.split(" -- ")[1]
    # if you get to the end of the file without finding the starting header
    rst_file.close()
    return rst_file_name.split('.')[0].replace('_', ' ')

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
master_doc = 'index'

extensions = [
    'sphinx.ext.napoleon',
    'sphinx_rtd_theme',
    'sphinx.ext.autosectionlabel',
    'sphinxemoji.sphinxemoji',
]

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = '.rst'

napoleon_google_docstring = True

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#

html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_logo = 'https://geopm.github.io/images/geopm-logo-clear.png'
logo_only = True

# -- Options for manual page output -------------------------------------------------

rst_files = [
    "geopm.7",
    "geopmadmin.1",
    "geopmagent.1",
    "geopm_agent_c.3",
    "geopm_agent_frequency_map.7",
    "geopm_agent_monitor.7",
    "geopm_agent_power_balancer.7",
    "geopm_agent_power_governor.7",
    "geopmbench.1",
    "geopmctl.1",
    "geopm_ctl_c.3",
    "GEOPM_CXX_MAN_Agent.3",
    "GEOPM_CXX_MAN_Agg.3",
    "GEOPM_CXX_MAN_CircularBuffer.3",
    "GEOPM_CXX_MAN_CNLIOGroup.3",
    "GEOPM_CXX_MAN_Comm.3",
    "GEOPM_CXX_MAN_CpuinfoIOGroup.3",
    "GEOPM_CXX_MAN_Daemon.3",
    "GEOPM_CXX_MAN_Endpoint.3",
    "GEOPM_CXX_MAN_Exception.3",
    "GEOPM_CXX_MAN_Helper.3",
    "GEOPM_CXX_MAN_IOGroup.3",
    "GEOPM_CXX_MAN_MonitorAgent.3",
    "GEOPM_CXX_MAN_MPIComm.3",
    "GEOPM_CXX_MAN_MSRIO.3",
    "GEOPM_CXX_MAN_MSRIOGroup.3",
    "GEOPM_CXX_MAN_PlatformIO.3",
    "GEOPM_CXX_MAN_PlatformTopo.3",
    "GEOPM_CXX_MAN_PluginFactory.3",
    "GEOPM_CXX_MAN_PowerBalancer.3",
    "GEOPM_CXX_MAN_PowerBalancerAgent.3",
    "GEOPM_CXX_MAN_PowerGovernor.3",
    "GEOPM_CXX_MAN_PowerGovernorAgent.3",
    "GEOPM_CXX_MAN_ProfileIOGroup.3",
    "GEOPM_CXX_MAN_SampleAggregator.3",
    "GEOPM_CXX_MAN_SharedMemory.3",
    "GEOPM_CXX_MAN_TimeIOGroup.3",
    "geopm_daemon_c.3",
    "geopmendpoint.1",
    "geopm_endpoint_c.3",
    "geopm_error.3",
    "geopm_fortran.3",
    "geopm_hash.3",
    "geopm_imbalancer.3",
    "geopmlaunch.1",
    "geopm_pio_c.3",
    "geopm_pio_dcgm.7",
    "geopm_pio_levelzero.7",
    "geopm_pio_nvml.7",
    "geopm_pio_sst.7",
    "geopm_policystore_c.3",
    "geopm_prof_c.3",
    "geopmpy.7",
    "geopmread.1",
    "geopm_report.7",
    "geopm_sched.3",
    "geopm_time.3",
    "geopm_topo_c.3",
    "geopm_version.3",
    "geopmwrite.1",
    "geopmaccess.1",
    "geopmsession.1",
    "geopmdpy.7"
]

authors = ["Christopher Cantalupo", "Brad Geltz", "Konstantin Rebrov"]
# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = []
for rst_file in rst_files:
    the_tuple = (
        rst_file,                           # startdocname
        rst_file.split('.')[0],             # name
        get_description(rst_file + ".rst"), # description
        authors,                            # authors
        int(rst_file.split('.')[1])         # section
    )
    man_pages.append(the_tuple)
