import mpi4py
import sys
import geopmpy.bench
import geopmpy.prof

# Example test that runs a mixed region
G_REPEAT = 500
G_REGIONS = ['stream', 'dgemm', 'stream', 'all2all']
G_BIG_O = [1.0, 1.0, 1.0, 1.0]

def root_print(msg):
    rank = mpi4py.MPI.COMM_WORLD.Get_rank()
    if rank == 0:
        sys.stdout.write(msg)

def main():
    is_verbose = '--verbose' in sys.argv or '-v' in sys.argv
    model_region_list = []
    for (region_name, big_o) in zip(G_REGIONS, G_BIG_O):
        region_name += '-unmarked'
        model_region_list.append(geopmpy.bench.model_region_factory(region_name, big_o, is_verbose))

    root_print('Beginning loop of {} iterations.\n'.format(G_REPEAT))

    region_id = geopmpy.prof.region('mixed', geopmpy.prof.REGION_HINT_UNKNOWN)
    for iter in range(G_REPEAT):
        geopmpy.prof.epoch()
        geopmpy.prof.region_enter(region_id)
        for model_region in model_region_list:
            geopmpy.bench.model_region_run(model_region)
        geopmpy.prof.region_exit(region_id)
        root_print('Iteration: {}\r'.format(iter))

if __name__ == '__main__':
    main()
