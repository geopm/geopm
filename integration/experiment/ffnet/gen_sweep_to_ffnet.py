#!/usr/bin/env python3

import argparse

import gen_hdf_from_fsweep as hdf
import gen_neural_net as nn
import gen_region_parameters as fmap

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output',
                       action='store',
                       help='Prefix of the output HDF and json file(s)',
                       default="neural_net_file")
    parser.add_argument('--description',
                       help='Description of the neural net, between quotes.',
                       dest="describe_net",
                       default="A neural net.")
    parser.add_argument('--ignore',
                       help='Comma-separated hashes of any regions to ignore.',
                       dest="region_ignore",
                       default=None)
    parser.add_argument('--frequency_sweep_dirs',
                        nargs='+',
                        help='Directories containing reports and traces from frequency sweeps')
    args = parser.parse_args()

    hdf.main(args.output, args.frequency_sweep_dirs)
    stats_hdf = f"{args.output}_stats.h5"
    trace_hdf = f"{args.output}_traces.h5"

    nn_out = f"{args.output}_nn"
    nn.main(trace_hdf, nn_out, args.describe_net, args.region_ignore)

    fmap_out = f"{args.output}_fmap"
    fmap.main(fmap_out, stats_hdf)

