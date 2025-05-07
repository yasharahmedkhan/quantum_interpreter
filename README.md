# The Quantum Interpreter
This is my attempt at an interpreter quantum computing programming langauge, for the purposes of my Computer Science senior project at Bard College. This is a functioning, albeit incomplete implementation. 

# Overview
This project allows you to:

1) Create and simulate quantum circuits using different algorithms
2) Compare the performance of various simulation techniques
3) Visualize quantum circuits and state information
4) Benchmark the simulator against different qubit counts and algorithms

The interpreter supports three different simulation approaches:

1) State Vector - Traditional full state vector simulation (exponential scaling)
2) Tensor Network - Matrix Product State (MPS) based simulation (better for low-entanglement circuits)
3) Stabilizer - Clifford circuit simulation using the stabilizer formalism (efficient for Clifford gates)

# Getting Started
Prerequisites:

1) GCC or compatible C compiler
2) Make build system
3) Standard C libraries (math, complex, etc.)

# Building

Clone the repository and build the project:
```bash
clone https://github.com/yourusername/quantum-interpreter.git
cd quantum-interpreter
make
```
This will create the quantum_interpreter executable.

# Using the Interpreter
Run the interpreter in interactive mode:

```bash
./quantum_interpreter
```

This will generate the command-line interface for the interpreter. For help on the specific commands, run:

```bash
> help
```

# Benchmarking

In order to run the benchmarking scripts, run the following command:

```bash
gcc -Wall -o simple_benchmark simple_benchmark.c -lm
```

This will create the benchmarking executable, which you can then run, for example:

```bash
./simple_benchmark 16 > benchmark_results.txt
```
These plots can then be visualized in the Jupyter notebook.

# TODO
This is an incomplete implementation. To contribute, the following changes and updates can be made: 

1) Fix tensor network implementation (current implementation has approximations)
2) Add more quantum gates (T, Rx, Ry, Rz, etc.)
3) Implement proper SWAP gate for tensor network algorithm
4) Optimize the stabilizer tableau operations
5) Add noise models for simulating real quantum hardware
6) Contribute to the grammar of a fully functioning quantum programming langauge

# License
This project is licensed under the MIT License - see the LICENSE file for details.

