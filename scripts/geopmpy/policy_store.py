#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

def connect(database_path):
    """Connect to the database at the given location.  Creates a new
    database if one does not yet exist at the given location.

    Args:
        database_path (str): Path to the database.
    """
    pass

def disconnect():
    """Disconnect the associated database.  No-op if the database has already
    been disconnected.
    """
    pass

def get_best(profile_name, agent_name):
    """Get the best known policy for a given agent/profile pair. If no best
    has been recorded, the default for the agent is returned.

    Args:
        profile_name (str): Name of the profile.
        agent_name (str): Name of the agent.

    Returns:
        list of float: Best known policy for the profile and agent.
    """
    pass

def set_best(profile_name, agent_name, policy):
    """ Set the record for the best policy for a profile with an agent.

    Args:
        profile_name (str): Name of the profile.
        agent_name (str): Name of the agent.
        policy (list of double): New policy to use.
    """
    pass

def set_default(agent_name, policy):
    """ Set the default policy to use with an agent.

    Args:
        agent_name (str): Name of the agent.
        policy (list of double): Default policy to use with the agent.
    """
    pass
