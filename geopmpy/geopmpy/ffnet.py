#!/usr/bin/python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import argparse
import json
import torch
import torch.nn.functional as F
import torch.utils.data as data

from geopmdpy import topo
from geopmdpy import pio

from geopmdpy.session import main
from geopmdpy.session import Agent

class FFNetAgent(Agent):
    def __init__(self):
        pass

    def update_parser(self, parser):
        parser.add_argument('--nnet',
                            help='Json containing Neural Net Data',
                            dest="nnet")
        parser.add_argument('--fmap',
                            help='Json containing frequency recommendation map',
                            dest="fmap")
        parser.add_argument('--model',
                            help='Output torch model of gen_neural_net.py',
                            dest='model')
        parser.add_argument('--perf-energy-bias',
                            help='Performance Energy Bias (0-1)',
                            default=0)
        return parser

    def update_args(self, args):
        self._nnet_path = args.nnet
        self._fmap_path = args.fmap
        self._model_path = args.model
        self._perf_energy_bias = args.perf_energy_bias
        return args

    def run_begin(self):
       self._ffnet_domain_pkg = FFNetDomain(topo.DOMAIN_PACKAGE, self._fmap_path,
                                            self._model_path, self._nnet_path,
                                            self._perf_energy_bias)
       self._ffnets = []
       for pkg in range(topo.num_domain(topo.DOMAIN_PACKAGE)):
            self._ffnets.append(FFNet(self._ffnet_domain_pkg, pkg))

    def update_loop(self):
        for ffnet in self._ffnets:
            ffnet.build_tensor()
            ffnet.forward()
            ffnet.calc_probabilities()
            ffnet.recommend_freq()

    def header_names(self):
        result = []
        for ffnet in self._ffnets:
            region_list = [f"{val}-{topo.domain_name(ffnet.ffnet_domain.domain_type)}-{ffnet.idx}"
                           for val in ffnet.ffnet_domain.nnet["trace_outputs"]]
            result.extend(region_list)
        return result

    def trace_out(self):
        return [str(val) for val in self._ffnet.probabilities]

class FFNetDomain:
    def __init__(self, domain_enum, fmap_path, model_path, nnet_path, perf_energy_bias=0):
        self.domain_type = domain_enum
        self.load_model(model_path)
        self.perf_energy_bias = perf_energy_bias
        self.freq_max = None
        self.freq_min = None
        self.load_ffnet_fmap(nnet_path, fmap_path)

    def load_model(self, model_path):
        #TODO: Add try/catch--look up common errors
        self.model = torch.jit.load(model_path)
        #Set model into evaluation mode
        self.model.eval()

    def load_ffnet_fmap(self, nnet_path, fmap_path):
        self.nnet = _load_json(nnet_path)
        self.fmap = _load_json(fmap_path)

        for region in self.nnet['trace_outputs']:
            #Check that regions in ffnet_data all have fmap
            if region not in self.fmap:
                raise KeyError(f"FFNet Error: Region {region} missing from freq map at {fmap_path}.")
            #Get frequency range from fmap
            if self.freq_max is None or self.freq_max < max(self.fmap[region]):
                self.freq_max = max(self.fmap[region])
            if self.freq_min is None or self.freq_min > min(self.fmap[region]):
                self.freq_min = min(self.fmap[region])



class FFNet:
    def __init__(self, ffnet_domain, domain_idx):
        self.ffnet_domain = ffnet_domain
        self.idx = domain_idx
        self.signal_names = []
        self.signal_idx = []
        self.samples = None
        self.output = None
        self.freq_ctl = {}
        self.init_signals()
        self.init_controls()

    def init_signals(self):
        for sample_name in self.ffnet_domain.nnet['signal_inputs']:
            if sample_name not in self.signal_names:
                self.signal_names.append(sample_name)
                self.signal_idx.append(pio.push_signal(sample_name,
                                                       self.ffnet_domain.domain_type,
                                                       self.idx))
        for num,dec in self.ffnet_domain.nnet['delta_inputs']:
            if num not in self.signal_names:
                self.signal_names.append(num)
                self.signal_idx.append(pio.push_signal(num,
                                                       self.ffnet_domain.domain_type,
                                                       self.idx))
            if dec not in self.signal_names:
                self.signal_names.append(dec)
                self.signal_idx.append(pio.push_signal(dec,
                                                       self.ffnet_domain.domain_type,
                                                       self.idx))
    def init_controls(self):
            ctl_min = {topo.DOMAIN_PACKAGE:"CPU_FREQUENCY_MIN_CONTROL",
                       topo.DOMAIN_GPU:"GPU_FREQUENCY_MIN_CONTROL"}
            ctl_min = {topo.DOMAIN_PACKAGE:"CPU_FREQUENCY_MAX_CONTROL",
                       topo.DOMAIN_GPU:"GPU_FREQUENCY_MAX_CONTROL"}
            #Check if min exists
            try:
                self.freq_ctl["min"] = pio.push_control(ctl_min[self.ffnet_domain.domain_type],
                                                        self.ffnet_domain.domain_type,
                                                        self.idx)
            except RuntimeError:
                pass
            self.freq_ctl["max"] = pio.push_control(ctl_min[self.ffnet_domain.domain_type],
                                                    self.ffnet_domain.domain_type,
                                                    self.idx)

    def build_tensor(self):
        self.samples = []
        #Simple reads
        for signal in self.ffnet_domain.nnet['signal_inputs']:
            idx = self.signal_names.index(signal)
            self.samples.append(pio.sample(self.signal_idx[idx]))
        #Ratios
        for num,dec in self.ffnet_domain.nnet['delta_inputs']:
            num_idx = self.signal_names.index(num)
            dec_idx = self.signal_names.index(dec)
            val = pio.sample(self.signal_idx[num_idx]) / pio.sample(self.signal_idx[dec_idx])
            self.samples.append(val)

    def forward(self):
        with torch.inference_mode():
            self.output = self.ffnet_domain.model(torch.Tensor(self.samples))
    def calc_probabilities(self):
            self.probabilities = F.softmax(self.output, dim=1)

    def recommend_freq(self):
        if len(self.ffnet_domain.nnet['trace_outputs']) != len(self.probabilities):
            raise RuntimeError(f"FFNet Error: Length of tensor from torch model does not match the number of trace_outputs in the nnet json.")
        freq = 0
        for idx, region in self.ffnet_domain.nnet['trace_outputs']:
            peb_idx = int((len(self.ffnet_domain.fmap[region]) - 1) * self.ffnet_domain.perf_energy_bias)
            freq += self.ffnet_domain.fmap[region][peb_idx] * self.probabilities[idx]

def _load_json(fpath):
    try:
        with open(fpath, 'r') as ff:
            data = json.load(ff)
        return data
    except FileNotFoundError:
        raise FileNotFoundError(f"FFNet Error: File not found at {f_path}")
    except json.JSONDecodeError as e:
        raise SyntaxError(f"FFNet Error: Invalid json found at {f_path}: {e}")
    except Exception as e:
        raise RuntimeError(f"FFNet Error: Error while loading json {f_path}: {e}")

if __name__ == '__main__':
    main(FFNetAgent())

