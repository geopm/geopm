#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

'''
Helpers for ploting scripts.
'''


def title_to_filename(title):
    fig_name = '{}'.format(title.lower()
                           .replace(' ', '_')
                           .replace(')', '')
                           .replace('(', '')
                           .replace(',', '')
                           .replace(':', '')
                           .replace('@', '_')
                           )
    return fig_name
