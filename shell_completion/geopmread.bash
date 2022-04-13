#!/bin/bash
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This is a bash completion script for geopmread. Source it in bash,
# or put it in your bash_completion.d directory

_geopmread_signals()
{
        cur=$1
        if [[ "$cur" != *":"* ]]
        then
                # If no colons are present in the current signal to complete,
                # the completion list is going to be way too long to read in
                # a single screen. Reduce verbosity by only showing signals
                # that have no prefix, and by showing only the set of prefixes
                # of other signals.
                geopmread 2>/dev/null | cut -d':' -f1-2 | sort | uniq
        else
                geopmread 2>/dev/null
        fi
}

_geopmread_options()
{
        # Extract single and double dash options from the help text, dropping equal signs
        geopmread $1 --help 2>/dev/null | tr ',' '\n' | awk -F '[\t =]+' '/^\s*(--\S+=?)|^\s*-\S/ {print $2}'
}

_geopmread_domains()
{
        signal=$1
        domains=$(geopmread $signal --info 2>/dev/null | awk '/domain:/ {print $2}')

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

_geopmread_domain_index()
{
        geopmread -d 2>&1 | awk -v domain="$1" '$0 ~ "^"domain"\\>" { for (i=0; i<$2; i++) print i }'
}

_geopmread()
{
        COMP_WORDBREAKS=${COMP_WORDBREAKS//:} # Let colons exist in a word
        COMPREPLY=()
        local cur
        local prev
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"

        case "$prev" in
                --info)
                        # The user wants info about a signal. Complete with the list of signals.
                        COMPREPLY=( $(compgen -W "$(_geopmread_signals $cur)" -- "$cur") )
                        ;;
                *)
                        if [ "$COMP_CWORD" -eq 1 ]
                        then
                                # Nothing entered yet. Complete with all options and
                                # with the list of signals.
                                COMPREPLY=( $(compgen -W "$(_geopmread_options) $(_geopmread_signals $cur)" -- "$cur") );
                        elif [ "$COMP_CWORD" -eq 2 ]
                        then
                                # An arg has been provided. Let's see if it is a signal.
                                # Try to complete with its domain or the --info option.
                                COMPREPLY=( $(compgen -W "$(_geopmread_domains $prev) --info" -- "$cur") );
                        elif [ "$COMP_CWORD" -eq 3 ]
                        then
                                # We have two previous args. Assume it is a signal and a domain.
                                # Complete with the possible domain indices
                                COMPREPLY=( $(compgen -W "$(_geopmread_domain_index $prev)" -- "$cur") );
                        fi
                        ;;
        esac

        # If the completion reply ends in a colon, we are probably looking at an
        # incomplete signal name. Don't auto-insert a space, since there is more
        # to the name.
        [[ $COMPREPLY == *':' ]] && compopt -o nospace
        return 0
}
complete -o nosort -F _geopmread geopmread
