#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Steady-state ResNet-50 FP16 inference benchmark.

Runs a batched inference loop on an Intel GPU (XPU, via Intel Extension
for PyTorch) or an NVIDIA GPU (CUDA), producing a sustained, compute-bound
GPU load near TDP.  Prints a single machine-parseable figure-of-merit line
to stdout so the GEOPM integration test can capture throughput regardless
of whether a control agent is active:

    FOM (images/sec): <float>

The model load, ``ipex.optimize``, and warmup iterations are excluded from
the timed region, so the reported throughput reflects steady-state
inference only.
"""

import argparse
import sys
import time


def _select_device(requested):
    """Resolve the compute device, importing IPEX for the XPU path."""
    import torch
    if requested == 'xpu':
        # Importing IPEX registers the 'xpu' device with torch.
        import intel_extension_for_pytorch  # noqa: F401
        if not torch.xpu.is_available():
            raise RuntimeError('requested device "xpu" but no XPU is available')
        return 'xpu'
    if requested == 'cuda':
        if not torch.cuda.is_available():
            raise RuntimeError('requested device "cuda" but no CUDA GPU found')
        return 'cuda'
    if requested == 'auto':
        try:
            import intel_extension_for_pytorch  # noqa: F401
            if torch.xpu.is_available():
                return 'xpu'
        except Exception:
            pass
        if torch.cuda.is_available():
            return 'cuda'
        raise RuntimeError('no XPU or CUDA GPU available for the "auto" device')
    return requested


def _synchronize(device):
    import torch
    if device == 'xpu':
        torch.xpu.synchronize()
    elif device == 'cuda':
        torch.cuda.synchronize()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--batch-size', type=int, default=64)
    parser.add_argument('--duration', type=float, default=60.0,
                        help='Timed run length in seconds.')
    parser.add_argument('--warmup', type=int, default=20,
                        help='Warmup iterations excluded from timing.')
    parser.add_argument('--device', default='auto',
                        choices=('auto', 'xpu', 'cuda', 'cpu'),
                        help='Compute device. Default: %(default)s.')
    args = parser.parse_args(argv)

    import torch
    import torchvision.models as models

    device = _select_device(args.device)

    model = models.resnet50(weights='ResNet50_Weights.DEFAULT').eval()
    model = model.to(device)
    if device == 'xpu':
        import intel_extension_for_pytorch as ipex
        model = ipex.optimize(model, dtype=torch.float16)
    data = torch.rand(args.batch_size, 3, 224, 224, device=device)
    if device in ('xpu', 'cuda'):
        data = data.half()
        model = model.half()

    count = 0
    with torch.no_grad():
        for _ in range(args.warmup):
            model(data)
        _synchronize(device)

        start = time.time()
        while time.time() - start < args.duration:
            model(data)
            count += 1
        _synchronize(device)
        elapsed = time.time() - start

    fom = (count * args.batch_size) / elapsed
    sys.stdout.write(f'FOM (images/sec): {fom:.3f}\n')
    sys.stdout.flush()
    return 0


if __name__ == '__main__':
    sys.exit(main())
