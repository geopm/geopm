
Guide for Contributors
======================

This file describes how to contribute to the GEOPM project.  All
feedback is appreciated.


Bug Report
----------

Please file an issue on our GitHub site for any bug observed:

https://github.com/geopm/geopm/issues

and select the "Bug report" template.

If possible, please reproduce issue with the --enable-debug option
passed to configure at build time and then report any error messages
emitted by GEOPM at run time.  Please provide the version of GEOPM
that produced the problem.  The GEOPM version can be found by running
the command

.. code-block::

   geopmread --version


or

.. code-block::

   geopmagent --version


Providing code that is small and reproduces the issue is nice to have.


Feature Request
---------------

Please submit an issue on GitHub for any feature requests using the
"Feature request" template.  Feature request issues should be titled
as a user story of the form:

   "As a <ROLE> I would like <FEATURE>"

in this way the author of the issue identifies the role that will
benefit from the feature and then describes the feature iteslf.
Common roles include "user of the GEOPM runtime", "user of the GEOPM
service", "developer of GEOPM", etc.  Please be as specific as
possible when identifying the role, e.g. "user of the geopmsession
command line tool" is prefered to "user of the GEOPM service".

When planning to implement a new feature as a contribution to the main
repository, please start this collaboration by creating a feature
request issue.  Please open the feature issue as soon as possible in
the process of planning the work.


Change Request
--------------

Changes to GEOPM are submitted by opening a pull request on GitHub
from a fork of GEOPM.  Including a Signed-off-by: line in the commit
message is appreciated, but not required.  All pull requests to the
geopm/geopm repository on GitHub will automatically be submitted to
GitHub Actions for unit testing.

The link for creating a pull request is here:

https://github.com/geopm/geopm/pulls

and the link to the results of the unit testing can be found under the
actions tab on the GEOPM GitHub webpage:

https://github.com/geopm/geopm/actions

We encourage users and developers of the GEOPM software to participate
in our code review process through GitHub.


Review process
^^^^^^^^^^^^^^

Our process for requesting a pull request to be reviewed and merged
is as follows:


#.
   An issue has been created about the problem that the pull request
   is solving.

#.
   Author creates a branch reflecting the issue number,
   e.g. "issue-1234".

#.
   Changes are made on that branch and pushed to the author's fork of
   the GEOPM repository.

#.
   The author submits the pull request based on the issue branch.
   Please refer to the Git documentation on how to create pull
   request:
   https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/about-pull-requests.
   Note that the pull request will be filled with a template.
   Selecting the "Preview" tab to render the markdown in the template
   may be helpful when editing the pull request text.

#.
   The author will edit the pull request template to include only one
   of the ``<details>`` sections.  Each section applies to a category
   of change.  Select the category that best fits the change you are
   making and delete all other details sections from the template.
   Make sure all check boxes are marked and all bracketed text is
   filled in for what remains of the template.

#.
   The submitter may optionally request specific people to be
   reviewers.  Otherwise, any GEOPM maintainer may be an approver for
   the pull request.  The ``#code-review`` channel on GEOPM's slack
   organization may be used to alert maintainers of a review request.

#.
   During the review process, a reviewer may request changes to the
   pull request.  If possible, these changes should be submitted as
   new commits without rebasing or squashing to simplify the reader's
   ability to view changes.

#.
   When the pull request is accepted by the reviewer(s), it will be
   marked "Approve".  Once the request is approved by all reviewers
   and all CI checks are passing, the change will be integrated into
   the GEOPM development branch by a maintainer.


Developers Guide
----------------

The guide for developers is maintained here:

https://geopm.github.io/geopmdpy/devel.html

Refer to this guide to learn how to make changes to the GEOPM source
code.
