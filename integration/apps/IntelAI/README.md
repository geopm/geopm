The Model Zoo for Intel Architecture is included here as an example
of running some machine learning models with GEOPM.

Currently this is an incomplete integration, using a basic approach
of adding mpi4py to the lauch_benchmark.py script for GEOPM monitoring.
This has only been tested on a single node with resnet50 using the
synthetic and coco inputs.

A better long term implementation is to swap the mpirun calls in
MODEL_ROOT/benchmarks/common/base_model_init.py, however this posed
problematic.

Please see the README.md at https://github.com/IntelAI/models/
