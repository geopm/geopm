# Feed Forward Agent

## Installation

``
make
``

## Usage

To use the agent, you need a neural net and 
### Generating a Neural Net
Use dump.py to convert a pytorch neural net into a format useable by LocalNeuralNet:


``
dump.py <path to input pytorch net> <path to output net>
``

### Set Up Environment
Set GEOPM_CPU_NN_PATH to the path of the neural net.

E.g.
``
export GEOPM_CPU_NN_PATH=${GEOPM_WORKDIR}/nets/net.out
``

