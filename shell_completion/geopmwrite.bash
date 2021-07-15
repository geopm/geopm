#!/bin/bash
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

# This is a bash completion script for geopmwrite. Source it in bash,
# or put it in your bash_completion.d directory

_geopmwrite_controls()
{
        cur=$1
        if [[ "$cur" != *":"* ]]
        then
                # If no colons are present in the current control to complete,
                # the completion list is going to be way too long to write in
                # a single screen. Reduce verbosity by only showing controls
                # that have no prefix, and by showing only the set of prefixes
                # of other controls.
                geopmwrite 2>/dev/null | cut -d':' -f1-2 | sort | uniq
        else
                geopmwrite 2>/dev/null
        fi
}

_geopmwrite_options()
{
        # Extract single and double dash options from the help text, dropping equal signs
        geopmwrite $1 --help 2>/dev/null | tr ',' '\n' | awk -F '[\t =]+' '/^\s*(--\S+=?)|^\s*-\S/ {print $2}'
}

_geopmwrite_domains()
{
        control=$1
        domains=$(geopmwrite $control --info 2>/dev/null | awk '/domain:/ {print $2}')

        if [[ "$domains" == *"cpu"* ]]; then
                domains="$domains core"
        fi
        if [[ "$domains" == *"core"* ]]; then
                domains="$domains package"
        fi
        if [[ "$domains" == *"package"* ]]; then
                domains="$domains board"
        fi

        echo "$domains"
}

_geopmwrite_domain_index()
{
        geopmwrite -d 2>&1 | awk -v domain="$1" '$0 ~ "^"domain"\\>" { for (i=0; i<$2; i++) print i }'
}

_geopmwrite()
{
        COMP_WORDBREAKS=${COMP_WORDBREAKS//:} # Let colons exist in a word
        COMPREPLY=()
        local cur
        local prev
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"

        case "$prev" in
                --info)
                        # The user wants info about a control. Complete with the list of controls.
                        COMPREPLY=( $(compgen -W "$(_geopmwrite_controls $cur)" -- "$cur") )
                        ;;
                *)
                        if [ "$COMP_CWORD" -eq 1 ]
                        then
                                # Nothing entered yet. Complete with all options and
                                # with the list of controls.
                                COMPREPLY=( $(compgen -W "$(_geopmwrite_options) $(_geopmwrite_controls $cur)" -- "$cur") );
                        elif [ "$COMP_CWORD" -eq 2 ]
                        then
                                # An arg has been provided. Let's see if it is a control.
                                # Try to complete with its domain or the --info option.
                                COMPREPLY=( $(compgen -W "$(_geopmwrite_domains $prev) --info" -- "$cur") );
                        elif [ "$COMP_CWORD" -eq 3 ]
                        then
                                # We have two previous args. Assume it is a control and a domain.
                                # Complete with the possible domain indices
                                COMPREPLY=( $(compgen -W "$(_geopmwrite_domain_index $prev)" -- "$cur") );
                        fi
                        ;;
        esac

        # If the completion reply ends in a colon, we are probably looking at an
        # incomplete control name. Don't auto-insert a space, since there is more
        # to the name.
        [[ $COMPREPLY == *':' ]] && compopt -o nospace
        return 0
}
complete -o nosort -F _geopmwrite geopmwrite
