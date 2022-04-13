#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import datetime as dt
import subprocess
import json
import os
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import time
import sys

class Query:
    def __init__(self,
                 labels=None,
                 state=None,
                 create_dates=None,
                 closed_dates=None):
        self.url = 'https://api.github.com/search/issues'
        self.repo = 'geopm/geopm'
        self.type = 'issue'
        if type(labels) is str:
            self.labels = [labels]
        else:
            self.labels = list(labels)
        self.state = state
        self.create_dates = create_dates
        self.closed_dates = closed_dates

    def __repr__(self):
        result = '{}?q='.format(self.url)
        result += 'repo:{}+type:{}'.format(self.repo, self.type)
        if self.state:
            result += '+state:{}'.format(self.state)
        for ll in self.labels:
            result += '+label:{}'.format(ll)
        if self.create_dates:
            result += '+created%3A{}..{}'.format(self.create_dates[0].isoformat(),
                                                 self.create_dates[1].isoformat())
        if self.closed_dates:
            result += '+closed%3A{}..{}'.format(self.closed_dates[0].isoformat(),
                                                self.closed_dates[1].isoformat())
        return result

    def auth(self):
        with open(os.path.expanduser('~') + '/.github-token') as fid:
           result = fid.read().strip()
        return result

    def query(self):
        # Sure would be nice to use the requests module, but I can't
        # get authentication to work!
        time.sleep(0.1)
        cmd = ['curl', '-u', self.auth(), str(self)]
        pid = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = pid.communicate()
        try:
            result = json.loads(out)
        except ValueError:
            result = {}
        return result

    def total_count(self):
        is_done = False
        while not is_done:
            qq = self.query()
            try:
                result = qq['total_count']
                if type(result) is not int:
                    raise TypeError
                is_done = True
            except (KeyError, TypeError):
                sys.stderr.write('Warning: Query failed\n')
                time.sleep(5)
        return result

def bug_combos():
    combos = [('bug-quality-low', 'bug-priority-low', 'bug-exposure-low'),
              ('bug-quality-low', 'bug-priority-low', 'bug-exposure-high'),
              ('bug-quality-low', 'bug-priority-high', 'bug-exposure-low'),
              ('bug-quality-low', 'bug-priority-high', 'bug-exposure-high'),
              ('bug-quality-high', 'bug-priority-low', 'bug-exposure-low'),
              ('bug-quality-high', 'bug-priority-low', 'bug-exposure-high'),
              ('bug-quality-high', 'bug-priority-high', 'bug-exposure-low'),
              ('bug-quality-high', 'bug-priority-high', 'bug-exposure-high')]
    legend = ['ql-pl-el',
              'ql-pl-eh',
              'ql-ph-el',
              'ql-ph-eh',
              'qh-pl-el',
              'qh-pl-eh',
              'qh-ph-el',
              'qh-ph-eh']
    return combos, legend

def format_pie(pct, allvals):
    total = sum(allvals)
    absolute = int(total * pct / 100.0 + 0.5)
    return "{:d}\n({:.1f}%)".format(absolute, pct)

def pie_chart(good_count, bad_count, title, good_name, bad_name):
    dd = [good_count, bad_count]
    plt.pie(dd, labels=[good_name, bad_name], colors = ['#33cc33','#ff5050'], startangle=45,
            autopct=lambda pct: format_pie(pct, dd))
    plt.axis('equal')
    plt.title(title)

def make_pies():
    # Make pies
    categories = [('bug-quality-high', 'bug-quality-low', 'Bug Quality'),
                  ('bug-priority-low', 'bug-priority-high', 'Bug Priority'),
                  ('bug-exposure-low', 'bug-exposure-high', 'Bug Exposure')]
    plt_idx = 1
    plt.figure(figsize=(11,4))
    today = dt.date.today()
    for cc in categories:
        plt.subplot(1,3,plt_idx)
        if plt_idx == 2:
            plt.xlabel(today.isoformat())
        good_label = cc[0]
        good_name = good_label.split('-')[-1]
        bad_label = cc[1]
        bad_name = bad_label.split('-')[-1]
        title = cc[2]
        query = Query(labels=good_label, state='open')
        good_count = query.total_count()
        query = Query(labels=bad_label, state='open')
        bad_count = query.total_count()
        pie_chart(good_count, bad_count, title, good_name, bad_name)
        plt_idx += 1
    plt.savefig('bug-pie.png')
    plt.close()

def make_age():
    begin = dt.date(2017, 1, 1)
    today = dt.date.today()
    delta30 = dt.timedelta(days=30)
    delta1 = dt.timedelta(days=1)
    date_ranges = [[today -     delta30, today               + delta1],
                   [today - 2 * delta30, today -     delta30 + delta1],
                   [today - 3 * delta30, today - 2 * delta30 + delta1],
                   [begin,               today - 3 * delta30 + delta1]]
    combos, combos_legend = bug_combos()
    count_matrix = []
    for cc in combos:
        count = []
        for rr in date_ranges:
            query = Query(labels=cc, state='open', create_dates=rr)
            count.append(query.total_count())
        count_matrix.append(tuple(count))

    date_names = ['00-30 days', '31-60 days', '61-90 days', '90+ days']
    bar_pos = range(len(date_names))
    bar_width = 0.75

    bottom = [0 for nn in date_names]
    for cc in count_matrix:
        plt.bar(bar_pos, cc, bottom=bottom, width=bar_width)
        bottom = [aa + bb for (aa, bb) in zip(bottom, cc)]
    plt.xticks(bar_pos, date_names)
    plt.ylabel('Open Bug Count')
    today = dt.date.today()
    plt.title('GEOPM Bug Aging by Quality-Priority-Exposure ({})'.format(today.isoformat()))
    plt.legend(combos_legend)
    plt.savefig('bug-age.png')
    plt.close()

def make_trend():
    long_ago = dt.date(2017, 1, 1)
    begin = dt.date(2020, 4, 1)
    today = dt.date.today()
    delta = dt.timedelta(days=14)
    date = [begin]
    open_count = []
    closed_count = []
    # Enable today as last date
    do_append = 2
    while do_append != 0:
        dates = (long_ago, date[-1])
        query = Query(labels='bug', create_dates=dates)
        total_count = query.total_count()
        query = Query(labels='bug', closed_dates=dates)
        closed_count.append(query.total_count())
        open_count.append(total_count - closed_count[-1])
        next_date = date[-1] + delta
        if next_date > today:
            do_append -= 1
        if do_append == 2:
            date.append(next_date)
        elif do_append == 1:
            date.append(today)

    plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%m/%d/%Y'))
    plt.gca().xaxis.set_major_locator(mdates.MonthLocator())
    plt.stackplot(date, open_count, closed_count,
                  colors=['#ff5050','#33cc33'], labels=['open', 'closed'])
    plt.gcf().autofmt_xdate()
    plt.legend(bbox_to_anchor=(0.01,0.99), loc='upper left')
    plt.xlabel('Total bug count')
    plt.title('GEOPM Bug Status Trend ({})'.format(today.isoformat()))
    plt.savefig('bug-trend.png')
    plt.close()

def make_cumulative():
    long_ago = dt.date(2017, 1, 1)
    today = dt.date.today()
    delta = dt.timedelta(days=14)
    date = []
    combos, combos_legend = bug_combos()
    count_matrix = []
    for weeks_ago in range(6):
        date.insert(0, today - weeks_ago * delta)

    date_names = []
    for dd in date:
        date_names.append(dd.strftime("%m/%d/%Y"))

    for cc in combos:
        count = []
        for dd in date:
            query = Query(labels=cc, create_dates=[long_ago, dd])
            count.append(query.total_count())
        count_matrix.append(tuple(count))

    plt.figure(figsize=(8,6))
    bar_pos = range(len(date))
    bar_width = 0.75
    bottom = [0 for dd in date]
    for cc in count_matrix:
        plt.bar(bar_pos, cc, bottom=bottom, width=bar_width)
        bottom = [aa + bb for (aa, bb) in zip(bottom, cc)]
    plt.xticks(bar_pos, date_names, rotation=30)
    plt.legend(combos_legend, bbox_to_anchor=(0.945, 0.5), loc='center left')
    plt.ylabel('Cumulative Bug Count')
    plt.title('GEOPM Cumulative Bugs by Quality-Priority-Exposure ({})'.format(today.isoformat()))
    plt.savefig('bug-cumulative.png')
    plt.close()


if __name__ == '__main__':
    make_pies()
    make_age()
    make_trend()
    make_cumulative()
