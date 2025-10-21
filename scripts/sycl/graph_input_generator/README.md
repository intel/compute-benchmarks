# Tool: Graph Input Generator

This tool translates GraphViz dot files generated from SYCL Graphs
(using `command_graph::print_graph()`) in real applications into a C++ header
containing a basic graph structure.
This header can be included in test files to easily generate new SYCL Graphs
with a similar structure to real applications and provide more realistic
testing of the graph implementation.

This tool primarily translates the nodes in the graph and the dependencies
between them and is not concerned with replicating the functionality of the
original workload. This is useful for testing aspects of graphs unrelated to
the actual execution of the graph such as finalize or graph building.

The tool converts the dot graph representation to an array of C++ structs,
with one array per input graph, which are inserted into the `graph_import.h.in`
template. The template header also defines the struct and enum types required
for this representation.

## Python Requirements

Requirements can be installed with `pip` via the provided `requirements.txt`.
The graphviz package needs to be installed. For example, in Ubuntu:

```
sudo apt install graphviz-dev
```

## Limitations

- Supported node types
  - Kernel
  - Barrier
- Any unsupported node types will be treated as kernel nodes.

## Usage

Example invocation:

```
python3 graph_dot_to_cpp.py --input_graphs gromacs.dot llama.dot \
--out <compute_benchmark_dir>/source/benchmarks/graph_api_benchmark/definition
```

## Structure sources

### Gromacs

The Gromacs graph was generated with the following command:

```bash
SYCL_CACHE_PERSISTENT=1 ONEAPI_DEVICE_SELECTOR=level_zero:gpu \
UR_L0_CMD_BUFFER_USE_IMMEDIATE_APPEND_PATH=1 \
gmx mdrun -pme gpu -pmefft gpu -s pme.tpr -nb gpu -update gpu -bonded gpu \
-nstlist 100 -ntomp 4 -notunepme -resetstep 2000  -nobackup -noconfout -pin on 
```

Gromacs was built from this branch: <https://gitlab.com/gromacs/gromacs/-/merge_requests/4954>
and was configured with the following cmake options:

```
-DGMX_GPU=SYCL \
-DGMX_SYCL_ENABLE_GRAPHS=ON \
-DGMX_FFT_LIBRARY=MKL \
-DGMX_GPU_FFT_LIBRARY=MKL \
-DGMX_SYCL_ENABLE_EXPERIMENTAL_SUBMIT_API=ON \
-DGMX_GPU_NB_CLUSTER_SIZE=8 \
-DCMAKE_BUILD_TYPE=Release \
-DGMX_GPU_NB_NUM_CLUSTER_PER_CELL_X=1
```

### Llama.cpp

The Llama.cpp graph was generated with the following commands:

```
GGML_SYCL_DISABLE_GRAPH=0 ONEAPI_DEVICE_SELECTOR=level_zero:gpu \
./bin/llama-cli -no-cnv -m <path-to-llama-models>/llama.cpp-models/DeepSeek-R1-Distill-Qwen-1.5B-Q4_0.gguf \
-ngl 81 -n 256 -c 1024 --no-mmap -sm none --color -p "What are the top 10 most beautiful countries?"
```

Llama was built with changes from <https://github.com/ggml-org/llama.cpp/pull/12972> 
to enable oneDNN usage in the graph.

Graph was taken from the 100th finalized graph as the first graphs are smaller and less representative.

## Adding a new graph structure

- Regenerate the header passing all the required graphs.
- For any new structures extend the GraphStructure enum and argument headers
  with the new option in `source/framework/argument`.
- Extend `source/benchmarks/graph_api_benchmark/implementations/sycl/helpers.h`
  to handle the new case and pass the appropriate structure array to the
  construction method.
- The name of the std::array objects in the generated header will match the
  input filename for each graph.

## Extending for other node types

This tool could easily be extended with other node types if required:

- Extend the `graph_import::NodeType` enum with new types.
- Modify the python script to match these new types from the dot node label.
- Extend the graph construction helpers in
  `source/benchmarks/graph_api_benchmark/implementations/sycl/helpers.h`
  to handle the new node types.
