#!/usr/bin/env python
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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


import sys
import os
import textwrap
import glob
import yattag
import argparse
import csv
import json
from collections import defaultdict

def add_license(doc):
    license_text = textwrap.dedent(''' \
    <!DOCTYPE html>
    <!--
    #  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
      -->
    ''')
    doc.asis(license_text)


def add_head(doc):
    with doc.tag('head'):
        doc.asis('<meta name="viewport" content="width=device-width, initial-scale=1">')
        with doc.tag('style'):
            # TODO: indentation looks funky here
            stylesheet = textwrap.dedent('''
            img {
            display: block;
            margin-left: auto;
            margin-right: auto;
            border:1px solid black;
            }
            h2 {text-align: center; font-family: sans-serif;}
            .note {max-width: 750px; margin: 0 auto;}
            .table_container {max-width: 1000px; margin: 0 auto;}
            h3 {text-align: center;}
            table, th, td {border:1px solid black; border-collapse: collapse;}
            th, td {padding: 5px;}
            .thumb_container {
            display: inline-block;
            }
            ''')
            doc.asis(stylesheet)


class ResultInfo:
    def __init__(self, path, description, thumbnail):
        self.path = path
        self.description = description
        self.thumbnail = thumbnail


def make_toc(doc, app_summary, exp_data):
    # with doc.tag('div', klass='note'):
    #     for link_name in items:
    #         with doc.tag('a', href='#{}'.format(link_name)):
    #             doc.text(link_name)
    #         doc.stag('br')

    # TODO: list of experiments should come from input file
    experiments = ['power_balancer_energy', 'frequency_sweep',
                   'balancer_comparison', 'barrier_frequency_sweep']
    apps = ['nekbone', 'minife', 'gadget', 'hpcg', 'nasft']
    with doc.tag('div', klass='table_container'):
        #doc.line('h3', 'TOC')
        with doc.tag('table'):
            with doc.tag('tr'):
                doc.stag('td')
                doc.line('th', 'summary')
                for exp in experiments:
                    doc.line('th', exp)
            for app in apps:
                with doc.tag('tr'):
                    doc.line('td', app)
                    try:
                        doc.line('td', app_summary[app])
                    except:
                        doc.line('td', 'no summary')
                    for exp in experiments:
                        with doc.tag('td'):
                            if (app, exp) in exp_data:
                                if exp == 'balancer_comparison':
                                    anchor = '#{}_{}'.format(app, 'power_balancer_energy')
                                else:
                                    anchor = '#{}_{}'.format(app, exp)
                                with doc.tag('a', 'data', href=anchor):
                                    thumb = exp_data[(app, exp)].thumbnail
                                    if len(thumb) > 0:
                                        rdir = exp_data[(app, exp)].path
                                        thumb = os.path.join(rdir, 'figures', thumb)
                                        doc.stag('img', src=thumb)#, width='100px')
                                    else:
                                        doc.line('p','data')
                            else:
                                doc.line('p', 'N/A')


def discover_experiments():
    summary_file = os.path.join('data', 'summary.json')
    app_summary = {}
    experiments = {}
    try:
        with open(summary_file) as infile:
            summary = json.load(infile)
            for app in summary["apps"]:
                name = app["name"]
                app_summary[name] = app["summary"]
                exp = app["experiments"]
                for exp_type, jj in exp.items():
                    thumbnail = jj["thumbnail"]
                    experiments[(name, exp_type)] = ResultInfo(path=os.path.join('data', jj["subdir"]),
                                                               description=jj["description"],
                                                               thumbnail=thumbnail)
    except Exception as ex:
        sys.stderr.write('Warning: {} not found or bad format: {}'.format(summary_file, ex))
    return app_summary, experiments


def benchmark_results(doc, key, data, extra):
    bench_name, exp_type = key
    if exp_type == 'balancer_comparison':
        return
    anchor = bench_name + '_' + exp_type
    doc.line('h3', bench_name + '_' + exp_type, id=anchor)
    with doc.tag('div', klass='note'):
        rdir = data.path
        # discover text tables
        # TODO: super brittle
        # TODO: 'tables' subdirectory
        stat_logs = glob.glob(os.path.join(rdir, 'figures', '*.log'))
        # discover plots in figures subdir
        images = glob.glob(os.path.join(rdir, 'figures', '*.png'))
        thumbs = glob.glob(os.path.join(rdir, 'figures', '*.gif'))
        with doc.tag('p'):
            with doc.tag('b'):
                doc.text('{}: '.format(rdir))
            with doc.tag('a', href=rdir):
                doc.text('raw data')
        doc.line('p', data.description)
        for log in stat_logs:
            with doc.tag('a', href=log):
                doc.text(os.path.basename(log))
            doc.stag('br')
        # main image
        main_img = data.thumbnail.replace('.gif', '.png')
        main_img = os.path.join(rdir, 'figures', main_img)
        doc.stag('img', src=main_img)
        if extra is not None:
            main_img = extra.thumbnail.replace('.gif', '.png')
            main_img = os.path.join(rdir, 'figures', main_img)
            doc.stag('img', src=main_img)
        # backup images
        for img in images:
            with doc.tag('div', klass='thumb_container'):
                img_thumb = img.replace('.png', '.gif')
                if img != main_img:
                    with doc.tag('a', href=img):
                        doc.stag('img', src=img_thumb, alt='FIXME', title=img)
        doc.stag('br')


class SmokeStatus:
    def __init__(self, path, run_status, analysis_status):
        self.path = path
        self.run_status = run_status
        self.analysis_status = analysis_status


def weekly_status(doc):
    experiments = ['monitor', 'power_sweep', 'frequency_sweep', 'energy_efficiency']
    apps = ['dgemm_tiny', 'dgemm', 'nekbone', 'minife', 'gadget', 'amg', 'hpcg']
    status = {}
    status_file = os.path.join('smoke', 'status.csv')
    try:
        with open(status_file) as infile:
            reader = csv.DictReader(infile, delimiter=',')
            for row in reader:
                if (row['exp_type'], row['app']) in status:
                    sys.stderr.write('Warning: multiple results for {} with {}'.format(row['exp_type'], row['app']))
                status[(row['exp_type'], row['app'])] = SmokeStatus(run_status=row['run_status'],
                                                                    analysis_status=row['analysis_status'],
                                                                    path=row['path'])
    except Exception as ex:
        sys.stderr.write('Warning: {} missing or wrong format: {}'.format(status_file, ex))

    with doc.tag('div', klass='note'):
        doc.line('h3', 'Integration smoke tests')
        doc.line('p', 'Check that: 1) run scripts produce the expected reports; ' +
                      '2) reports can be used with analysis scripts for the experiment type to produce the expected plots and tables. ' +
                      'No evaluation of performance.')
        # TODO: pandas df to table
        with doc.tag('table'):
            #doc.stag('col')
            # header row: experiment type
            with doc.tag('tr'):
                doc.stag('td', rowspan=2)
                for exp in experiments:
                    doc.line('th', exp, colspan=3)
            # header row: test type
            with doc.tag('tr'):
                for exp in experiments:
                    doc.line('th', 'run script')
                    doc.line('th', 'analysis')
                    doc.line('th', 'raw data')
            for app in apps:
                with doc.tag('tr'):
                    doc.line('td', app)
                    for exp in experiments:
                        if (exp, app) in status:
                            doc.line('td', status[(exp, app)].run_status)
                            doc.line('td', status[(exp, app)].analysis_status)
                            with doc.tag('td'):
                                doc.line('a', 'data', href=status[(exp, app)].path)
                        else:
                            doc.line('td', 'not run', colspan=3, style='text-align: center;')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true', default=False)
    args = parser.parse_args()

    doc = yattag.Doc()
    add_license(doc)
    with doc.tag('html'):
        add_head(doc)
        with doc.tag('body'):
            doc.line('h2', 'Experiments')
            app_summary, experiments = discover_experiments()
            make_toc(doc, app_summary, experiments)

            for key, jobs in experiments.items():
                if key[1] == 'power_balancer_energy':
                    extra = experiments[(key[0], 'balancer_comparison')]
                else:
                    extra = None
                benchmark_results(doc, key, jobs, extra)

            weekly_status(doc)

    result = yattag.indent(doc.getvalue())
    debug = True
    if args.debug:
        sys.stdout.write(result + '\n')
    with open('index.html', 'w') as outfile:
        outfile.write(result)
