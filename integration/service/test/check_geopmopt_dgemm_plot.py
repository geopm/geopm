
from geopmdpy.grid import ControlGrid
from parse import parse
from matplotlib import pyplot as plt
import sys

if len(sys.argv) < 2:
    sys.stderr.write(f'Usage: {sys.argv[0]} LOGFILE\n\n')
    sys.exit(-1)

with open(sys.argv[1]) as fid:
    content = fid.read()

xx = []
yy = []
grid_argv = None
for ll in content.splitlines():
    pp = parse('INFO: Evaluation {}: coordinate=[{}], metric={}', ll)
    if pp:
        xx.append(int(pp[1]))
        yy.append(float(pp[2]))
    elif ll.startswith('+ geopmopt'):
        grid_argv = ll.split()[1:]

x_label = None
if grid_argv is not None and False:
    grid = ControlGrid(grid_argv)
    xx_update = []
    for x in xx:
        config = grid.get_config([x])
        xx_update.append(config[0][3])
        x_label = config[0][0]
    xx = xx_update
plt.plot(xx, yy, 'x')
if x_label is not None:
    plt.x_label(x_label)
plt.savefig('check_geopmopt_dgemm_plot.png')
