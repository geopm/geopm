#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
copyright = '2021, Intel (R) Corporation'
author = 'Intel (R) Corporation'

# -- Data structures ---------------------------------------------------------

geopm_pages = {}
external_pages = {}

# -- Helper functions --------------------------------------------------------

# return a list of all files in the directory ronn_dir
def get_ronn_files_list(ronn_dir):
    ronn_glob = f"{ronn_dir}/*.ronn"
    return [ os.path.basename(rg) for rg in glob.glob(ronn_glob) ]

# return a list of all files in ronn_files, but replace the "ronn" with "rst" at the end
def get_rst_files_list(ronn_files):
    return [ rf.replace(".ronn", ".rst") for rf in ronn_files ]

# adjust the names of man pages to have links
def adjust_links(line):
    if line.find("**#include") != -1:
        line = line.replace("**", "")
    for a, href in geopm_pages.items():
        format_text = f"**{a}**"
        format_link = f"`{a} <{href}.html>`_"
        line = line.replace(format_text, format_link)
    for a, href in external_pages.items():
        format_text = f"**{a}**"
        format_link = f"`{a} <{href}>`_"
        line = line.replace(format_text, format_link)
    return line

# remove the extra lines copyright leftovers from the rst files
def rewrite_file(filename):
    newfilename = filename + ".temp"
    print(f"processing {filename} ...")
    with open(filename) as oldfile, open(newfilename, 'w') as newfile:
        key_string = """:raw-html-m2r:`<a href="#" title="OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.">//</a>`\ : # ()"""
        for line in oldfile:
            if line.find(key_string) == -1:
                newfile.write(adjust_links(line))
    oldfile.close()
    newfile.close()
    os.system(f"mv {newfilename} {filename}")

# read the index.txt and fill the geopm_pages and external_pages
def fill_links_table(ronn_dir):
    filename = os.path.join(ronn_dir, "index.txt")
    global links_table
    section = ""
    with open(filename) as index_txt:
        # loop through each line of text
        for line in index_txt:
            line = line.rstrip()
            if (section == "#geopm pages"):
                if (line.isspace() or line == ""):
                    section = ""
                else:
                    line_split = line.split()
                    geopm_pages[line_split[0]] = line_split[1]
            elif (section == "#external pages"):
                if (line.isspace() or line == ""):
                    section = ""
                else:
                    line_split = line.split()
                    external_pages[line_split[0]] = line_split[1]
            else:  # section == ""
                if (line == "#geopm pages"):
                    section = "#geopm pages"
                elif (line == "#external pages"):
                    section = "#external pages"
    index_txt.close()

# print the geopm_pages and external_pages
def print_links_table():
    print("geopm_pages")
    for key, value in geopm_pages.items():
        print(key, value, sep = '\t')
    print("external_pages")
    for key, value in external_pages.items():
        print(key, value, sep = '\t')

# Use m2r tool to create the rst files from the ronn files,
# and process the rst files, removing incorrect extra lines.
def create_rst_files(ronn_files, rst_files):
    for ronn_file, rst_file in zip(ronn_files, rst_files):
        os.system(f"m2r {ronn_dir}/{ronn_file}")
        os.system(f"mv {ronn_dir}/{rst_file} .")
        rewrite_file(rst_file)

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.napoleon',
    'sphinx_rtd_theme'
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

# relative path to geopm/ronn from geopm/service/docs/source
ronn_dir = "../../../ronn"
# It is a list of ronn files in the directory geopm/ronn
ronn_files = get_ronn_files_list(ronn_dir)

# It is a list of rst files corresponding to the ronn files.
rst_files = get_rst_files_list(ronn_files)

fill_links_table(ronn_dir)

create_rst_files(ronn_files, rst_files)

rst_files = [ rf.replace(".rst", "") for rf in rst_files ]

authors = ["Christopher Cantalupo", "Brad Geltz"]
# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = []
for rst_file in rst_files:
    the_tuple = (
        rst_file,                                 # startdocname
        rst_file.split('.')[0],                   # name
        rst_file.split('.')[0].replace('_', ' '), # description
        authors,                                  # authors
        int(rst_file.split('.')[1])               # section
    )
    man_pages.append(the_tuple)
