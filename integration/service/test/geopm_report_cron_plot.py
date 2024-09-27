#!/usr/bin/env python3
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from yaml import load_all as yaml_load_all
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader
import sys
import os
from pandas import DataFrame, to_datetime, to_timedelta
import plotly.graph_objs as go
from dash import Dash, dcc, html
from argparse import ArgumentParser

class CronReport:
    def __init__(self, report_paths):
        report_list = []
        for rn in report_paths:
            with open(rn) as fid:
                report_docs = yaml_load_all(fid, Loader=SafeLoader)
                report_docs = list(report_docs)
            if len(report_docs) == 0:
                sys.stderr.write(f'Warning: {rn} is empty, could not be parsed\n')
            for report in report_docs:
                df_row_keys = set(report) - {'metrics'}
                df_row = dict()
                for df_key in df_row_keys:
                    if df_key == 'sample-time-first':
                        df_row[df_key] = to_datetime(report[df_key])
                    else:
                        df_row[df_key] = report[df_key]
                for metric_name, metrics in report['metrics'].items():
                    df_row['metric-name'] = metric_name
                    for m_key, m_value in metrics.items():
                        df_row[m_key] = m_value
                    report_list.append(df_row.copy())
        if len(report_list) == 0:
            raise RuntimeError(f'No files could be parsed')
        self._df = DataFrame(report_list)
        self._df.sort_values('sample-time-first', inplace=True)
        if len(self._df) == 0:
            raise RuntimeError('Could not load reports')
        self._colors = dict(
            mean_marker='rgb(247, 240, 25)',
            min_marker='rgb(82, 115, 222)',
            max_marker='rgb(217, 85, 85)',
            std_marker='rgb(68, 68, 68)',
            std_fill='rgba(68, 68, 68, 0.5)',
            range_fill='rgba(68, 68, 68, 0.3)',
            pie_line='rgb(0, 0, 0)',
            pie_gpu='red',
            pie_cpu='orange',
            pie_dram='yellow',
            background='rgb(41, 128, 185)',
            page_text='rgb(252, 252, 252)',
        )
        self._font_family = 'Lato, proxima-nova, Helvetica Neue, Arial, sans-serif'


    def calculate_energy(self, domain, begin_date, end_date):
        """Return an estimate of energy for all reports that begin within the time range
        specified.  Only includes time prior to the beginning of the last report
        that matches.

        """
        # This calculation uses only the start time of each report and the
        # average power measured between report start times.  This avoids any
        # double counting in case reports overlap, and avoids undercounting when
        # there are gaps between the reports.
        #
        # TODO A side effect is that power measurements from the last report are
        # ignored, and not used in the calculation, this could be corrected.
        # Also consider that partial reports are also not used in this method.
        domain = domain.upper()
        metric_name = f'{domain}_POWER'
        time = self.get_stat(begin_date, end_date, metric_name, 'sample-time-first')
        time_total = self.get_stat(begin_date, end_date, metric_name, 'sample-time-total')
        power = self.get_stat(begin_date, end_date, metric_name, 'mean')
        kwh_factor = 1.0 / 3.6e6
        return kwh_factor * sum((power.shift(1) * (time - time.shift(1)).dt.total_seconds()).dropna())

    def dataframe(self):
        return self._df

    def date_range(self):
        """Return oldest and newest report 'sample-time-first'.

        Does not infer the ISO date that the last report extends to based on
        'sample-time-total'.
        """
        # TODO: Consider returning last report's last sample time as the second
        # value instead of the last report's first sample time
        return (min(self._df['sample-time-first']),
                max(self._df['sample-time-first']))

    def get_stat(self, begin_date, end_date, metric_name, stat_name):
        return self._df.loc[self._df['sample-time-first'].between(begin_date, end_date) & \
                            (self._df['metric-name'] == metric_name), stat_name]

    def plot_power(self, domain, begin_date, end_date):
        domain = domain.upper()
        metric_name = f'{domain}_POWER'
        sample_time_first = self.get_stat(begin_date, end_date, metric_name, 'sample-time-first')
        sample_time_total = self.get_stat(begin_date, end_date, metric_name, 'sample-time-total')
        record_mean_time = sample_time_first + to_timedelta(sample_time_total / 2.0, unit='s')
        record_min_power = self.get_stat(begin_date, end_date, metric_name, 'min')
        record_max_power = self.get_stat(begin_date, end_date, metric_name, 'max')
        record_mean_power = self.get_stat(begin_date, end_date, metric_name, 'mean')
        record_std_power = self.get_stat(begin_date, end_date, metric_name, 'std')
        record_plus_std = [min((a, b)) for (a, b) in zip(record_mean_power + record_std_power, record_max_power)]
        record_minus_std = [max((a, b)) for (a, b) in zip(record_mean_power - record_std_power, record_min_power)]
        fig = go.Figure([
            go.Scatter(
                name=f'{domain} Power Min',
                x=record_mean_time,
                y=record_min_power,
                mode='lines',
                marker=dict(color=self._colors['min_marker']),
                line=dict(width=1),
            ),
            go.Scatter(
                name=f'{domain} Power Max',
                x=record_mean_time,
                y=record_max_power,
                mode='lines',
                marker=dict(color=self._colors['max_marker']),
                fillcolor=self._colors['range_fill'],
                line=dict(width=1),
                fill='tonexty',
            ),
            go.Scatter(
                name=f'{domain} Power Std',
                x=record_mean_time,
                y=record_plus_std,
                mode='lines',
                marker=dict(color=self._colors['std_marker']),
                line=dict(width=0),
                showlegend=False,
            ),
            go.Scatter(
                name=f'{domain} Power Std',
                x=record_mean_time,
                y=record_minus_std,
                mode='lines',
                marker=dict(color=self._colors['std_marker']),
                line=dict(width=0),
                fillcolor=self._colors['std_fill'],
                fill='tonexty',
            ),
            go.Scatter(
                name=f'{domain} Power Average',
                x=record_mean_time,
                y=record_mean_power,
                mode='lines',
                marker=dict(color=self._colors['mean_marker']),
                line=dict(width=1),
            ),
        ])
        fig.update_layout(
            yaxis_title=f'{domain} Power (w)',
            title=f'{domain} Power',
            hovermode='x'
        )
        return fig

    def plot_energy(self, begin_date, end_date):
        labels = ['GPU', 'CPU', 'DRAM']
        values = [self.calculate_energy("gpu", begin_date, end_date),
                  self.calculate_energy("cpu", begin_date, end_date),
                  self.calculate_energy("dram", begin_date, end_date)]
        colors = [self._colors['pie_gpu'],
                  self._colors['pie_cpu'],
                  self._colors['pie_dram']]
        fig = go.Figure(data=[go.Pie(labels=labels, values=values)])
        fig.update_traces(hoverinfo='label+percent', textinfo='value', textfont_size=20,
                          marker=dict(colors=colors, line=dict(color=self._colors['pie_line'], width=2)))
        begin_string = f'{begin_date.year}/{begin_date.month}/{begin_date.day}'
        end_string = f'{end_date.year}/{end_date.month}/{end_date.day}'
        fig.update_traces(texttemplate='%{value:.3}')
        fig.update_layout(title=f'Energy - kWh ({begin_string} - {end_string})', hovermode='x')
        return fig

    def run_webpage(self, begin_date, end_date, port, host_mask):
        app = Dash('geopmsession')
        style={
            'textAlign': 'center',
            'color': self._colors['page_text'],
            'font-family': self._font_family
        }
        app.layout = html.Div(style={'backgroundColor': self._colors['background']}, children=[
            html.H1(children='GEOPM Power Report', style=style),
            dcc.Graph(id='gpu-power',
                      figure=self.plot_power('gpu', begin_date, end_date)),
            dcc.Graph(id='cpu-power',
                      figure=self.plot_power('cpu', begin_date, end_date)),
            dcc.Graph(id='dram-power',
                      figure=self.plot_power('dram', begin_date, end_date)),
            dcc.Graph(id='energy-pie',
                      figure=self.plot_energy(begin_date, end_date)),
        ])
        app.run(host=host_mask, port=port)

    def static_webpage(self, begin_date, end_date, output_html):
        if (not output_html.endswith('.html')):
            raise RuntimeError(f'Output file {output_html} does not end in .html')
        header = f'<h1 style="text-align: center; color: {self._colors["page_text"]}; background-color: {self._colors["background"]}; '\
                 f'font-family: {self._font_family};">GEOPM Power Report</h1>'
        with open(output_html, 'w') as fid:
            fid.write('<html>\n    <body>\n')
            fid.write(header)
            for domain in ('gpu', 'cpu', 'dram'):
                fig = self.plot_power(domain, begin_date, end_date)
                fid.write(fig.to_html(full_html=False, default_height='450px', include_plotlyjs='cdn'))
            fig = self.plot_energy(begin_date, end_date)
            fid.write(fig.to_html(full_html=False, default_height='450px', include_plotlyjs='cdn'))
            fid.write('    </body>\n</html>\n')

def get_parser():
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('report', nargs='+')
    static_group = parser.add_mutually_exclusive_group()
    static_group.add_argument('-s', '--static-html', dest='static_html', default=None,
                              help='Do not run web server, output static html into directory provided')
    static_group.add_argument('-p', '--port', dest='port', type=int, default=10850,
                              help='Port opened by server to provide visualization. Default: %(default)s')
    parser.add_argument('-m', '--host-mask', dest='host_mask', default='0.0.0.0',
                        help='Restrict connections based on IP address. Default: no restrictions')
    return parser

def main():
    """Command line interface for creating geopmsession report visualizations

    Parses a collection of geopmsession YAML reports and creates an HTML
    visualiation.  By default it will open a dash server on port 10850.  Can
    also create static web pages with the ``--static-html`` option.

    """
    err = 0
    try:
        args = get_parser().parse_args()
        cron_report = CronReport(args.report)
        begin_date, end_date = cron_report.date_range()
        if args.static_html is None:
            cron_report.run_webpage(begin_date, end_date, args.port, args.host_mask)
        else:
            cron_report.static_webpage(begin_date, end_date, args.static_html)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    exit(main())
