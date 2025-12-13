
from geopmdpy.grid import ControlGrid
from parse import parse
from matplotlib import pyplot as plt
import numpy as np
import sys

if len(sys.argv) < 2:
    sys.stderr.write(f'Usage: {sys.argv[0]} LOGFILE\n\n')
    sys.exit(-1)

with open(sys.argv[1]) as fid:
    content = fid.read()

plot_data = {0: ([], [], []),
             1: ([], [], []),
             2: ([], [], []),
             4: ([], [], []),
             8: ([], [], []),
             16: ([], [], []),
             32: ([], [], [])}
grid_argv = None
intensities = sorted(plot_data.keys())
ii = 0
xx = []
yy = []
zz = []
for ll in content.splitlines():
    pp = parse('INFO: Evaluation {}: coordinate=[{}, {}], metric={}', ll)
    if pp:
        xx.append(int(pp[1]))
        yy.append(int(pp[2]))
        zz.append(float(pp[3]))
    elif ll.startswith('+ geopmopt'):
        grid_argv = ll.split()[2:]
        grid_argv = grid_argv[:grid_argv.index('--verbosity=2')]
    if 'Optimization completed!' in ll:
        plot_data[intensities[ii]] = (xx, yy, zz)
        ii += 1
        xx = []
        yy = []
        zz = []

plt.figure(figsize=(8, 10.5))
plt_idx = 1
extent = [0.8, 3.5, 0.8, 2.3]
for aib_int in intensities[1:]:
    xx, yy, zz = plot_data[aib_int]
    max_x = max(xx) + 1
    max_y = max(yy) + 1
    data = np.full((max_y, max_x), np.nan)
    for x, y, z in zip(xx, yy, zz):
        data[max(yy) - y][x] = z / 1000
    plt.subplot(3, 2, plt_idx)
    plt.imshow(data, extent=extent, cmap='nipy_spectral', aspect='auto', interpolation='none')
    if plt_idx > 4:
        plt.xlabel('CPU Freq (GHz)')
    if plt_idx % 2 == 1:
        plt.ylabel('Uncore Freq (GHz)')
    plt.colorbar(label='Energy (kJ)')
    plt.title(f'AIB Intensity {aib_int}')
    plt_idx += 1
plt.savefig('check_geopmopt_aib_map.png')
