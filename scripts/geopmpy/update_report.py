#
#  Copyright (c) 2015 - 2022, Intel Corporation
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
import shutil

is_new_host = False
is_epoch_section = False
is_app_section = False


def update_report_line(old_line):
    global is_new_host
    global is_app_section
    global is_epoch_section
    result = []
    # Keep track of global state
    if old_line.startswith('Host: '):
        is_new_host = True
        is_app_section = False
        is_epoch_section = False
    if old_line.startswith('Region ') and is_new_host:
        result.append('    Regions:')
        is_new_host = False
        is_app_section = False
        is_epoch_section = False
    if old_line.startswith('Epoch Totals:'):
        is_new_host = False
        is_app_section = False
        is_epoch_section = True
    if old_line.startswith('Application Totals:'):
        is_new_host = False
        is_app_section = True
        is_epoch_section = False

    # Define rules for rearranging report
    if old_line.startswith('Host: '):
        host_name = old_line.replace('Host: ', '')
        result.append('  {}:'.format(host_name))
    elif old_line == 'Region unmarked-region (0x00000000725e8066):':
        result.append('    Unmarked Totals:')
    elif old_line in ('Epoch Totals:', 'Application Totals:'):
        result.append('    {}'.format(old_line))
    elif old_line.startswith('Region '):
        name_hash = old_line[len('Region '):-2]
        sep = name_hash.rfind(' (')
        name = name_hash[:sep]
        hash = name_hash[sep + 2:]
        result.append('    -')
        result.append('      region: "{}"'.format(name))
        result.append('      hash: {}'.format(hash))
    elif old_line.startswith('    epoch-runtime-ignore (sec):'):
        result.append('  {}'.format(old_line.replace('epoch-runtime-ignore', 'time-hint-ignore')))
    elif old_line.startswith('    ignore-time (sec):'):
        result.append('  {}'.format(old_line.replace('ignore-time', 'time-hint-ignore')))
    elif old_line.startswith('    network-time (sec):'):
        result.append(old_line.replace('network-time', '  time-hint-network'))
        if not (is_app_section or is_epoch_section):
            result.append('      time-hint-ignore (s): 0')
        result.extend(['      time-hint-compute (s): 0',
                       '      time-hint-memory (s): 0',
                       '      time-hint-io (s): 0',
                       '      time-hint-serial (s): 0',
                       '      time-hint-parallel (s): 0',
                       '      time-hint-unknown (s): 0',
                       '      time-hint-unset (s): 0'])
    elif is_app_section and old_line.startswith('    power (watts):'):
        result.append('  {}'.format(old_line))
        result.append('      frequency (%): 0')
        result.append('      frequency (Hz): 0')
    elif is_app_section and old_line.startswith('    runtime (sec):'):
        time = float(old_line.split(': ')[1])
        result.append('  {}'.format(old_line))
        result.append('      sync-runtime (s): {}'.format(time))
    elif old_line.startswith('    geopmctl memory HWM:'):
        num_byte = int(old_line.split(': ')[1].replace(' kB', '')) * 1024
        result.append('      geopmctl memory HWM (B): {}'.format(num_byte))
    elif old_line.startswith('Figure of Merit:'):
        result.append(old_line)
    elif old_line.startswith('    '):
        # any other data inside regions, epoch totals or app totals
        result.append('  {}'.format(old_line))
    elif not old_line.startswith(' ') and is_new_host:
        # deal with agent host extensions
        result.append('    {}'.format(old_line))
    elif old_line == '':
        result.append(old_line)
    else:
        raise RuntimeError("Line not handled: " + old_line)
    result = [ll.rstrip()
              .replace('0x00000000', '0x')
              .replace('(sec)', '(s)')
              .replace('(joules)', '(J)')
              .replace('(watts)', '(W)') for ll in result]
    return result


def update_report_str(old_report):
    old_report_lines = old_report.splitlines()
    new_report_lines = []
    if len(old_report_lines) < 6:
        raise ValueError('Input is missing header')
    version = old_report_lines[0]
    version = version.replace('#####','')
    version = version.replace(' geopm', 'GEOPM Version:')
    new_report_lines.append(version.strip())
    header_keys = ['Start Time: ', 'Profile: ', 'Agent: ']
    for (key, line) in zip(header_keys, old_report_lines[1:4]):
        if not line.startswith(key):
            raise ValueError('Unable to parse header')
    new_report_lines.extend(old_report_lines[1:4])
    if old_report_lines[4].startswith('Policy: '):
        new_report_lines.append(old_report_lines[4])
        blank_idx = 5
    else:
        blank_idx = 4
    if old_report_lines[blank_idx] != '':
        raise ValueError('Expected empty line after header')
    while old_report_lines[blank_idx + 1] == '':
        blank_idx += 1
    if not old_report_lines[blank_idx  + 1].startswith('Host: '):
        raise ValueError('Expected report to begin with a Host: ' + old_report_lines[blank_idx  + 1])
    new_report_lines.append('')
    new_report_lines.append('Hosts:')
    for line in old_report_lines[blank_idx + 1:]:
        new_report_lines.extend(update_report_line(line))
    return '\n'.join(new_report_lines)


def is_old_format(path):
    with open(path) as fid:
        first_line = fid.readline()
    return first_line.startswith('##### geopm 1.')


def update_report(in_file, out_file=None):
    is_in_place = (in_file == out_file or out_file is None)
    if is_old_format(in_file):
        if is_in_place:
            backup = '{}.orig'.format(in_file)
            if os.path.exists(backup):
                raise RuntimeError('Tried to convert file, but backup already exists')
            shutil.copyfile(in_file, backup)
            out_file = in_file
        with open(in_file) as fid:
            report_str = fid.read()
            updated = update_report_str(report_str)
        with open(out_file, 'w') as fid:
            fid.write(updated)
    elif not is_in_place:
        shutil.copyfile(in_file, out_file)


if __name__ == '__main__':
    usage = """\
Usage: {} in_file [out_file]

    in_file: Path of report file in original format

    out_file: Optional output path of converted file, if not
              specified, then input is overwritten.
""".format(sys.argv[0])

    if len(sys.argv) == 2:
        update_report(sys.argv[1])
    elif len(sys.argv) == 3:
        update_report(sys.argv[1], sys.argv[2])
    else:
        sys.stderr.write(usage)
