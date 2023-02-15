#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
