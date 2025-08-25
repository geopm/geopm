Contributor Guide
=================

This file describes how to contribute to the GEOPM project: how to create bug
reports, request features, and contribute to the repository.  All feedback is
appreciated.

Refer to the developers guide for more details about how to make changes to the
source code.  The guide for developers is maintained here:

https://geopm.github.io/devel.html


Bug Report
----------

Please file an issue on our GitHub site for any bug observed:

https://github.com/geopm/geopm/issues/new/choose

and select the "Bug report" template.

If possible, please reproduce issue with the --enable-debug option
passed to configure at build time and then report any error messages
emitted by GEOPM at run time.  Please provide the version of GEOPM
that produced the problem.  The GEOPM version can be found by running
the command

.. code-block::

   geopmread --version


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
command line tool" is preferred to "user of the GEOPM service".

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

Making a pull request is a step in a process.  The first step in this
process is creating an issue from one the "Bug report", "Feature
request", or "Story" templates.  This step identifies the high level
goal for one or more changes to the repository.

Each change to the repository is identified with an issue created
using one of the issues templates that begins "Change - ".  The
"Change" issue should reference the "Bug report," "Feature request,"
or "Story" that the change relates to.  This is done with a "Relates
to #XXXX" note in the issue description.

Each pull request will close a "Change" issue by filling in the "Fixes
#XXXX" notation in the pull request template, it may also close the
high level issue, if this is the last required change to fix the bug,
complete the feature, or accomplish the story described in the high
level issue.  Note that the requirements for the pull request should
be documented in the "Change" issue, not in the pull request itself.

The link for creating an issue is here:

https://github.com/geopm/geopm/issues/new/choose

The link for creating a pull request is here:

https://github.com/geopm/geopm/pulls

The link to the results of the unit testing can be found under the
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
   is solving.  Note that most PRs that change source code refer to
   an issue created with one of the "Change - " templates.

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
   Check that all of the requirements in the issue referenced by the
   PR have been met.

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

Generative AI Usage
-------------------

Code or other content generated in whole or in part using generative
AI tools can be contributed to the GEOPM project. However,
contributors must adhere to the following guidelines, which are based
on the Linux Foundation's recommendations:

- Ensure that the terms and conditions of the generative AI tool do
  not impose any restrictions on the output that conflict with the
  project's open source license, intellectual property policies, or
  the Open Source Definition.
- If the AI-generated output includes pre-existing copyrighted
  materials (including open source code) authored or owned by third
  parties, contributors must confirm they have permission to use,
  modify, and contribute such materials. This permission must comply
  with the project's licensing policies.
- Provide notice and attribution for any third-party materials
  included in the contribution, along with information about the
  applicable license terms.
- When possible, use AI tools that offer features to suppress or flag
  responses similar to third-party materials, and review flagged
  content for compliance with licensing requirements.

Contributors should also comply with their employer's policies
regarding the use of AI tools for software development. For more
details, refer to the Linux Foundation's guidelines on generative AI
usage: https://www.linuxfoundation.org/legal/generative-ai
