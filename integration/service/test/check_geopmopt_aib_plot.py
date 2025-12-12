
from geopmdpy.grid import ControlGrid
from parse import parse
from matplotlib import pyplot as plt
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

plt_idx = 1
num_int = len(intensities)
plt.figure(figsize=(8, 10.5))
for aib_int in intensities:
    xx, yy, zz = plot_data[aib_int]
#    grid = ControlGrid(grid_argv)
#    xx_update = []
#    yy_update = []
#    for x, y in zip(xx, yy):
#        config = grid.get_config([x, y])
#        xx_update.append(config[0][3])
#        yy_update.append(config[1][3])
#    x_label = config[0][0]
#    y_label = config[1][0]
#    xx = xx_update
#    yy = yy_update
    plt.subplot(num_int, 2, plt_idx)
    plt.plot(xx, zz, 'x')
    plt.title(f'AIB {aib_int} Core Freq')
    plt.ylabel('Energy (J)')
    plt_idx += 1
    plt.subplot(num_int, 2, plt_idx)
    plt.plot(yy, zz, 'x')
    plt.title(f'AIB {aib_int} Uncore Freq')
    plt.ylabel('Energy (J)')
    plt_idx += 1

plt.savefig('check_geopmopt_aib_plot.png')
