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

# This is a bash completion script for geopmlaunch. Source it in bash,
# or put it in your bash_completion.d directory

_geopmlaunch_launchers()
{
        # Extract the part of the help text (parts separated by empty lines)
        # that lists the launchers. Strip out quotation marks and replace
        # commas with newlines
        geopmlaunch --help 2>/dev/null | awk -F':' -e 'BEGIN { RS="\n\n" } /Possible LAUNCHER values:/ { gsub(/(\"|,|[[:space:]])+/, "\n", $2); print $2 }'
}

_geopmlaunch_options()
{
        # Extract single and double dash options from the help text, dropping equal signs
        geopmlaunch $1 --help 2>/dev/null | tr ',' '\n' | awk -F '[\t =]+' '/^\s*(--\S+=?)|^\s*-\S/ {print $2}'
}

_geopmlaunch()
{
        COMPREPLY=()
        local cur
        local prev
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"

        [[ "$prev" == '=' ]] && prev="${COMP_WORDS[COMP_CWORD-2]}"

        case "$prev" in
                --geopm-agent)
                        COMPREPLY=( $(compgen -W "$(geopmagent 2>/dev/null)" -- "${cur#=}") )
                        return 0
                        ;;
                --geopm-ctl)
                        COMPREPLY=( $(compgen -W "process pthread application" -- "${cur#=}") )
                        return 0
                        ;;
                --geopm-trace-signals|--geopm-report-signals)
                        if [[ "$cur"  == *','* ]]
                        then
                                local last rest
                                last="${cur##*,}"
                                rest="${cur%,*}"
                                # There's a comma. Prefix each recommendation with the already-entered list.
                                COMPREPLY=( $(compgen -P "${rest}," -W "$(geopmread 2>/dev/null)" -- "${last#=}") )
                        else
                                COMPREPLY=( $(compgen -W "$(geopmread 2>/dev/null)" -- "${cur#=}") )
                        fi
                        return 0
                        ;;
                *)
                        if [ "$COMP_CWORD" -eq 1 ]
                        then
                                COMPREPLY=( $(compgen -W "$(_geopmlaunch_launchers)" -- "${cur}") );
                        else
                                if [[ ! " ${COMP_WORDS[@]:0:${COMP_CWORD}} " =~ " -- " ]]
                                then
                                        # Haven't encountered a -- argument
                                        local launcher="${COMP_WORDS[1]}"
                                        COMPREPLY=( $(compgen -W "$(_geopmlaunch_options $launcher)" -- "${cur}") );
                                fi
                        fi
                        [[ $COMPREPLY == *= ]] && compopt -o nospace
                        return 0
                        ;;
        esac
}
complete -o default -F _geopmlaunch geopmlaunch
