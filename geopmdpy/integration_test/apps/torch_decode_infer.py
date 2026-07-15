#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Memory-bandwidth-bound, batch-1 autoregressive "decode" benchmark.

Emulates the token-by-token decode phase of interactive LLM inference,
which is the archetypal *frequency-insensitive but busy* GPU workload:

  * Batch size 1 turns every projection into a tall-skinny GEMV whose
    runtime is bounded by weight-matrix memory bandwidth, not by compute
    throughput.  The compute engine is therefore only partially active
    (low ``GPU_CORE_ACTIVITY``) even though the GPU reports itself busy
    (high ``GPU_UTILIZATION``).
  * Each generated token depends on the previous one, so the driver issues
    a host synchronize per token -- exactly like a real autoregressive
    decode loop -- adding short host-side gaps.

This is the scenario where the GPU activity agent is most differentiated
from the GPU's own hardware DVFS: hardware DVFS keeps the clock high
because the GPU looks busy, but performance is nearly insensitive to core
frequency, so the agent can lower the frequency (via the
``activity / utilization`` ratio) and save energy with negligible loss.

A synthetic transformer-style stack of large linear layers is used so the
benchmark is self-contained -- no model weights are downloaded and no
``transformers`` dependency is required.  Prints one figure-of-merit line:

    FOM (tokens/sec): <float>
"""

import argparse
import os
import sys
import time


def _select_device(requested):
    """Resolve the compute device, importing IPEX for the XPU path."""
    import torch
    if requested == 'xpu':
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


def _build_model(torch, hidden, layers):
    import torch.nn as nn

    class DecodeStack(nn.Module):
        def __init__(self, hidden, layers):
            super().__init__()
            # A stack of large square projections with a residual and a
            # non-linearity approximates a transformer block's per-token
            # matmul-bound work without attention over a growing context.
            self.blocks = nn.ModuleList(
                [nn.Linear(hidden, hidden, bias=False) for _ in range(layers)])
            self.act = nn.GELU()
            # Normalize each token so magnitudes stay bounded when the output
            # is fed back in for thousands of decode steps (avoids FP16
            # overflow); also adds realistic bandwidth-bound work.
            self.norm = nn.LayerNorm(hidden)

        def forward(self, x):
            for blk in self.blocks:
                x = x + self.act(blk(x))
            return self.norm(x)

    return DecodeStack(hidden, layers).eval()


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--duration', type=float, default=60.0,
                        help='Timed run length in seconds.')
    parser.add_argument('--warmup', type=int, default=20,
                        help='Warmup tokens excluded from timing.')
    parser.add_argument('--hidden', type=int,
                        default=int(os.environ.get('GEOPM_GPU_DECODE_HIDDEN',
                                                   8192)),
                        help='Model hidden dimension. Default: %(default)s.')
    parser.add_argument('--layers', type=int,
                        default=int(os.environ.get('GEOPM_GPU_DECODE_LAYERS',
                                                   8)),
                        help='Number of projection layers. Default: %(default)s.')
    parser.add_argument('--device', default='auto',
                        choices=('auto', 'xpu', 'cuda', 'cpu'),
                        help='Compute device. Default: %(default)s.')
    args = parser.parse_args(argv)

    import torch

    device = _select_device(args.device)
    model = _build_model(torch, args.hidden, args.layers).to(device)

    dtype = torch.float16 if device in ('xpu', 'cuda') else torch.float32
    if device == 'xpu':
        import intel_extension_for_pytorch as ipex
        model = ipex.optimize(model, dtype=dtype)
    model = model.to(dtype)

    # Batch size 1: the defining property that makes each layer a
    # bandwidth-bound GEMV rather than a compute-bound GEMM.
    token = torch.rand(1, args.hidden, device=device, dtype=dtype)

    count = 0
    with torch.no_grad():
        for _ in range(args.warmup):
            token = model(token)
            _synchronize(device)

        start = time.time()
        while time.time() - start < args.duration:
            # Feed each token's output back in, synchronizing per token to
            # mirror a real autoregressive decode dependency.
            token = model(token)
            _synchronize(device)
            count += 1
        elapsed = time.time() - start

    fom = count / elapsed
    sys.stdout.write(f'FOM (tokens/sec): {fom:.3f}\n')
    sys.stdout.flush()
    return 0


if __name__ == '__main__':
    sys.exit(main())
