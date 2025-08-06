 # Compute Benchmarks
A benchmark suite developed and maintained by NEO driver development team in order to provide a stable environment, which allows checking and reproducing performance for various parts of the driver. The benchmark suite is divided into multiple executables, each of which tests a different aspect of the driver. Each executable contained in ComputeBechmarks consists of multiple test cases with a set of predefined configurations (parameters like buffer sizes, workgroup sizes, etc.) along with a possibility to run them with any desired parameters. They support both OpenCL and Level Zero APIs, although some tests may be available in only one API.

Current list of tests can be found in [TESTS.md](TESTS.md).

## How to run benchmarks
For specific information about how to run the benchmarks, please run the binary with "--help" parameter.

## FAQ
Answers to frequently asked questions and common problems can be found in [FAQ.md](FAQ.md).


## Building


### Run CMake and compile
Build the benchmark as any other CMake project. Example command sequence:
```
git clone --recurse-submodules https://github.com/intel/compute-benchmarks
cd compute-benchmarks
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
The last line can be substituted with platform-specific command, for example:
```
make -j`nproc`
```
or:
```
make memory_benchmark_ocl -j`nproc`
```

### Building with SYCL support

#### Building with DPC++

SYCL implementations of benchmarks will be built if the following conditions are met:

* oneAPI is installed with support for DPC++;
* the user has sourced the `setvars.sh` script prior to running CMake;
* the CMake option `BUILD_SYCL` is set to `ON`;

Note that building SYCL benchmarks is currently only supported on Linux.

##### Example

```
git clone --recurse-submodules https://github.com/intel/compute-benchmarks
cd compute-benchmarks
mkdir build
cd build
. /opt/intel/oneapi/setvars.sh
cmake .. -DBUILD_SYCL=ON
cmake --build . --config Release
```

#### Building with AdaptiveCpp

The SYCL implementations of the benchmarks can also be built with AdaptiveCpp:

* `acpp` must be in your path;
* `SYCL_COMPILER_ROOT` must be set manually to point at AdaptiveCpp;
* `SYCL_COMPILER` must be set manually to point at AdaptiveCpp;
* `ALLOW_WARNINGS` must be set to `ON`;

It is recommended to set the optimization flags explicitly, because
AdaptiveCpp's defaults are less agressive than those of DPC++.

##### Example

```
git clone --recurse-submodules https://github.com/intel/compute-benchmarks
cd compute-benchmarks
mkdir build
cmake .. -DBUILD_SYCL=ON -DSYCL_COMPILER_ROOT=/path/to/AdaptiveCpp/install/ -DSYCL_COMPILER=acpp -DALLOW_WARNINGS=ON -DCMAKE_CXX_FLAGS="-O3"
cmake --build . --config Release
```


### Building with MPI support

Hybrid MPI + X implementations of the benchmarks will be built if:

* the CMake option `BUILD_MPI` is set to `ON`;
* CMake is able to locate the MPI compilers/headers/libraries through its `FindMPI` module;

For Intel MPI, setting `I_MPI_OFFLOAD=1` or `I_MPI_OFFLOAD=2` is required; for other MPI implementations please refer to their manuals.

The MPI benchmarks are only supported on Linux.

### Binary types
Each benchmark suite can be built as a single-api binary or as a an all-api binary.
- Single-api binaries are named like `ulls_benchmark_ocl` and do not load libraries from not used APIs. They are built by default and can be disabled by passing `-DBUILD_SINGLE_API_BINARIES=OFF` to CMake.
- All-api binaries are named like `ulls_benchamrk` and contain every API supported by the benchmark, which makes them more convenient, but at a risk of introducing some overhead caused by loading unnecessary library. They are *not* built by default and can be enabled by passing `-DBUILD_ALL_API_BINARIES=ON` to CMake.

By default all benchmarks are compiled in all configurations they support. This can be changed with CMake arguments. For example, to disable all-api binaries, run:
```
cmake .. -DBUILD_ALL_API_BINARIES=OFF -DBUILD_SINGLE_API_BINARIES=ON
```

### SDK
ComputeBenchmarks will try to find SDKs for the APIs used. In case of inability to find those, it will use libraries contained in [third_party/opencl-sdk](third_party/opencl-sdk) and [third_party/level-zero-sdk](third_party/level-zero-sdk) directories. The libraries were compiled on Ubuntu 20.04 LTS. Using a different setup may result in build failures due to ABI incompatibility, so it's safest to have the SDK installed in your system.

### Contributing
Information on how to contribute to ComputeBenchmarks can be found in [CONTRIBUTING.md](CONTRIBUTING.md)
