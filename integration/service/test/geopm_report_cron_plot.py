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
from glob import glob
from pandas import DataFrame, to_datetime, to_timedelta
import plotly.graph_objs as go
from dash import Dash, dcc, html
from argparse import ArgumentParser
from datetime import timedelta
from datetime import datetime

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


    def calculate_energy(self, domain, begin_date, end_date, host=None):
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
        time = self.get_stat(begin_date, end_date, metric_name, 'sample-time-first', host)
        time_total = self.get_stat(begin_date, end_date, metric_name, 'sample-time-total', host)
        power = self.get_stat(begin_date, end_date, metric_name, 'mean-arithmetic', host)
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

    def get_stat(self, begin_date, end_date, metric_name, stat_name, host=None):
        if host is None:
            return self._df.loc[self._df['sample-time-first'].between(begin_date, end_date) & \
                                (self._df['metric-name'] == metric_name), stat_name]
        else:
            return self._df.loc[self._df['sample-time-first'].between(begin_date, end_date) & \
                                (self._df['metric-name'] == metric_name) & \
                                (self._df['host'] == host), stat_name]

    def plot_power(self, domain, begin_date, end_date, host=None):
        domain = domain.upper()
        metric_name = f'{domain}_POWER'
        sample_time_first = self.get_stat(begin_date, end_date, metric_name, 'sample-time-first', host)
        sample_time_total = self.get_stat(begin_date, end_date, metric_name, 'sample-time-total', host)
        record_mean_time = sample_time_first + to_timedelta(sample_time_total / 2.0, unit='s')
        record_min_power = self.get_stat(begin_date, end_date, metric_name, 'min', host)
        record_max_power = self.get_stat(begin_date, end_date, metric_name, 'max', host)
        record_mean_power = self.get_stat(begin_date, end_date, metric_name, 'mean-arithmetic', host)
        record_std_power = self.get_stat(begin_date, end_date, metric_name, 'std', host)
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

    def plot_energy(self, begin_date, end_date, host=None):
        labels = ['GPU', 'CPU', 'DRAM']
        values = [self.calculate_energy("gpu", begin_date, end_date, host),
                  self.calculate_energy("cpu", begin_date, end_date, host),
                  self.calculate_energy("dram", begin_date, end_date, host)]
        colors = [self._colors['pie_gpu'],
                  self._colors['pie_cpu'],
                  self._colors['pie_dram']]
        fig = go.Figure(data=[go.Pie(labels=labels, values=values)])
        fig.update_traces(hoverinfo='label+percent', textinfo='value', textfont_size=20,
                          marker=dict(colors=colors, line=dict(color=self._colors['pie_line'], width=2)))
        begin_string = f'{begin_date.year}/{begin_date.month}/{begin_date.day}'
        end_string = f'{end_date.year}/{end_date.month}/{end_date.day}'
        fig.update_traces(texttemplate='%{value:.3}')
        if host is not None:
            title_host = f'{host} - '
        else:
            title_host = ''
        if begin_string != end_string:
            title_date = f'{begin_string} - {end_string}'
        else:
            title_date = begin_string
        fig.update_layout(title=f'{title_host}Energy (kWh): {title_date}', hovermode='x')
        return fig

    def plot_host_energy(self, begin_date, end_date, daily_url):
        host_list = self.get_hosts()
        host_energy = dict()
        dates = self.get_days(begin_date, end_date)
        delta = timedelta(hours=24)
        if daily_url is None:
            raise ValueError('daily_url option must be provided when creating a host energy heatmap')
        pub_path_format = '<a href="{daily_url}/{host}/{date}.html">{value}</a>'
        host_url = []
        host_energy = []
        for host in host_list:
            date_energy = []
            date_url = []
            for begin_interval in dates:
                end_interval = begin_interval + delta
                date_energy.append(self.calculate_energy("gpu", begin_interval, end_interval, host) +
                                   self.calculate_energy("cpu",  begin_interval, end_interval, host) +
                                   self.calculate_energy("dram",  begin_interval, end_interval, host))
                date_url.append(pub_path_format.format(daily_url=daily_url, host=host, date=begin_interval.date(), value=f'{date_energy[-1]:.2f}'))
            host_energy.append(list(date_energy))
            host_url.append(date_url)

        host_totals = [sum(he) for he in host_energy]
        # sort host_energy, host_list, and host_url in  order of total energy
        host_energy = [he for _, he in sorted(zip(host_totals, host_energy))]
        host_list = [hl for _, hl in sorted(zip(host_totals, host_list))]
        host_url = [hu for _, hu in sorted(zip(host_totals, host_url))]

        fig = go.Figure(data=go.Heatmap(z=host_energy, x=dates, y=host_list, text=host_url, texttemplate="%{text}", textfont={"size":20}, colorscale='pinkyl'))
        fig.update_layout(title='Daily Energy by Host (kWh)')
        return fig

    def get_hosts(self):
        host_list = self._df['host'].unique()
        host_list.sort()
        return host_list

    def get_days(self, begin_date, end_date):
        dates = [datetime.combine(begin_date.date(), datetime.min.time(), tzinfo=begin_date.tzinfo)]
        delta = timedelta(hours=24)
        # trim last two days
        end_date = end_date - 2 * delta
        while dates[-1] < end_date:
            dates.append(dates[-1] + delta)
        return dates

    def run_webpage(self, begin_date, end_date, port, host_mask, daily_url):
        app = Dash('geopmsession')
        style={
            'textAlign': 'center',
            'color': self._colors['page_text'],
            'font-family': self._font_family
        }
        app.layout = html.Div(style={'backgroundColor': self._colors['background']}, children=[
            html.H1(children='GEOPM Energy Report', style=style),
            dcc.Graph(id='host-energy',
                      figure=self.plot_host_energy(begin_date, end_date, daily_url)),
        ])

        app.run(host=host_mask, port=port)

    def static_webpage(self, begin_date, end_date, output_html, host_select=None, daily_url=None):
        if (not output_html.endswith('.html')):
            raise RuntimeError(f'Output file {output_html} does not end in .html')
        header = f'<h1 style="text-align: center; color: {self._colors["page_text"]}; background-color: {self._colors["background"]}; '\
                 f'font-family: {self._font_family};">GEOPM Energy Report</h1>'
        with open(output_html, 'w') as fid:
            fid.write('<html>\n    <body>\n')
            fid.write(header)
            if host_select is None:
                fig = self.plot_host_energy(begin_date, end_date, daily_url)
                fid.write(fig.to_html(full_html=False, include_plotlyjs='cdn'))
            else:
                fig = self.plot_energy(begin_date, end_date, host_select)
                fid.write(fig.to_html(full_html=False, default_height='450px', include_plotlyjs='cdn'))
                for domain in ('cpu', 'dram','gpu'):
                    fig = self.plot_power(domain, begin_date, end_date, host_select)
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
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-d', '--host-select', dest='host_select', default=None,
                        help='Only plot data gathered from one host')
    group.add_argument('-u', '--daily-url', dest='daily_url', default=None,
                       help="URL of directory containing a subdirectory for each host, e.g. if a report URL for my-host-1 is http://my.cluster.io/geopm-report/my-host-1/2024-08-28.html then specify --daily-url=http://my.cluster.io/geopm-report"
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
        if len(args.report) == 1 and '*' in args.report[0]:
            args.report = glob(args.report[0])
        cron_report = CronReport(args.report)
        begin_date, end_date = cron_report.date_range()
        if args.static_html is None:
            cron_report.run_webpage(begin_date, end_date, args.port, args.host_mask, args.daily_url)
        else:
            cron_report.static_webpage(begin_date, end_date, args.static_html, args.host_select, args.daily_url)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    exit(main())
