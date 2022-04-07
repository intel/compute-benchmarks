# Frequently asked questions



### Does it support Windows?
Yes.



### Does it support Linux?
Yes.



### What's the difference between memory_benchmark_ocl and memory_benchmark?
Binaries named with *_ocl* and *_l0* suffixes, such as *memory_benchmark_ocl*, are single-API benchmarks, meaning they enable tests in only one API and do not load unnecessary libraries. On the other hand *memory_benchmark* loads both OpenCL and Level Zero libraries and runs tests in both APIs for convenience. This may introduce some additional overhead, so it's best to use single-API binaries for serious tests.



### Do they work on any GPU?
Binaries have been designed to run on any platform support by the OpenCL and Level Zero drivers. If you get any errors or crashes, it is very likely to be related to improper configuration or some fundamental flaws in the drivers. If your environment is confirmed to be correct and you still experience problems, please file an issue.



### How does it work?
ComputeBenchmarks is divided into multiple binaries. Every binary tests a specific aspect of the GPU and/or driver. Every binary defines a set of named test cases, which can have multiple parameters. By default, each test is run with a set of default parameters, but you can select custom values. The tests are based on OpenCL and Level Zero APIs. Some of them may use Intel-specific extensions.



### How do I run all tests?
Select binary you want, for example memory_benchmark_ocl.exe and run it without any parameters.



### How do I run only XXX test?
Use the test filter. It will cause the program to only run benchmarks, whose name are specified. For example:
```
./memory_benchmark_ocl.exe --testFilter=ReadBuffer
./memory_benchmark_ocl.exe --testFilter="ReadBuffer"
./memory_benchmark_ocl.exe --testFilter="ReadBuffer WriteBuffer MapBuffer"
```



### How do I run only XXX, but only cases, where parameter YYY is equal to ZZZ?
Use the arg filter. It will cause the program to only run benchmarks, whose parameters match **all** of those specified. For example:
```
./memory_benchmark_ocl.exe --argFilter="useEvents=1"
./memory_benchmark_ocl.exe --testFilter="ReadBuffer" --argFilter="useEvents=1"
./memory_benchmark_ocl.exe --testFilter="ReadBuffer" --argFilter="useEvents=1 size=512MB"
```



### How do I run test XXX with custom parameters?
Run the benchmark with parameter --test specifying the name of the test followed by all parameters required by the test. For example:
```
.\memory_benchmark.exe --test=UsmSharedMigrateCpu --api=ocl --accessAllBytes=1 --size=128MB
```
For this you will have to know all the parameters for given test. There are two ways. The first one is to run the test with --help parameter and find your XXX test. All arguments for the test should be specified and described. The second (easier) way is to run the test with --dumpCommandLines argument, which will run the tests and print the ready to run command line arguments. For example
```
.\memory_benchmark.exe --dumpCommandLines --testFilter=XXX
```



### Can I import results to a spreadsheet easily?
Yes, run the benchmark with --csv parameter.



### What tests are supported?
Every benchmark binary can be run with --help parameter, which provides a short description for each test case. You can also check the documentation in [TESTS.md](TESTS.md).



### What does test XXX do?
Every benchmark binary can be run with --help parameter, which provides a short description for each test case. You can also check the documentation in [TESTS.md](TESTS.md).



### Why does CMake tell me, that Level Zero/OpenCL installation was not found?
No need to worry about it. CMake requires special config files, so it can discovoer installed libraries. Sometimes the files are not there, even if SDKs are installed correctly. For this reason the repository contains its own Level Zero SDK and OpenCL SDK (loader+headers), which makes it self-sufficient. If system-wide installation cannot be found, benchmarks are built using these SDKs and they are still functional.



### Why am I getting linker errors related to libze_loader.so?
You probably cloned the repository on Windows and copied the files over to Linux. Because of that symbolic links pointing to the Level Zero loader break and they are interpreted as raw files. Recommended way is to clone and build the repository on the same system. Or, if you don't care about LevelZero tests, you can disable them with `-DBUILD_L0=OFF` CMake argument.
