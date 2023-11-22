/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <emmintrin.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <string.h>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <intrin.h>
size_t getCachelineSize() {
    DWORD lpiCount = 0;
    GetLogicalProcessorInformation(nullptr, &lpiCount);

    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> lpis(lpiCount);
    GetLogicalProcessorInformation(lpis.data(), &lpiCount);

    for (const auto &lpi : lpis) {
        if (lpi.Relationship == LOGICAL_PROCESSOR_RELATIONSHIP::RelationCache && lpi.Cache.Level == 1) {
            return lpi.Cache.LineSize;
        }
    }
    return 0;
}
#else
#include <x86intrin.h>
size_t getCachelineSize() {
    return 64;
}
#endif

enum class CachelineFlushOp : uint32_t {
    MM_CLFLUSH = 0,
    MM_CLFLUSHOPT,
    MM_CLWB
};

enum class OverrideMemType : uint32_t {
    NONE = 0,
    ONES,
    RANDOM
};

constexpr size_t constCachelineSize = 64;

constexpr CachelineFlushOp defaultOpType = CachelineFlushOp::MM_CLFLUSHOPT;
constexpr OverrideMemType defaultOverrideMem = OverrideMemType::RANDOM;
constexpr size_t defaultComparisonIterations = 10;
constexpr size_t defaultClflushIterations = 1;
constexpr size_t defaultSizes[] = {1, 4, 8, 64, 1024, 4096, 2097152, 2147483648};
constexpr bool defaultAlign = false;
constexpr bool defaultPrecise = false;
constexpr bool defaultClSize = true;

void printHelp() {
    std::cout << "options:\n";
    std::cout << "-h : print help and quit\n";
    std::cout << "-s <NUMBER,NUMBER,...>. : set allocation sizes separated by , ,defaults are";
    for (const auto &size : defaultSizes) {
        std::cout << " " << size;
    }
    std::cout << "\n";
    std::cout << "-f [NONE,ONES,RANDOM] : fill memory between iterations, default is RANDOM \n";
    std::cout << "-i <NUMBER>: set iterations count, default is " << defaultComparisonIterations << "\n";
    std::cout << "-m <NUMBER> : set clflush count in single iteration, default is " << defaultClflushIterations << "\n";
    std::cout << "-a [0,1] : align pointer to cacheline, default is " << defaultAlign << "\n";
    std::cout << "-p [0,1] : substract cost of loops, default is " << defaultPrecise << "\n";
    std::cout << "-t [0,1] : use compile time cl size = 64, default is " << defaultClSize << "\n";
    std::cout << "-c [MM_CLFLUSH,MM_CLFLUSHOPT,MM_CLWB] : set clflush intrinsic type to be used, default is MM_CLFLUSHOPT \n";
}

void printTimes(std::vector<double> &vec) {
    auto sum = std::accumulate(vec.begin(), vec.end(), 0.0);
    auto avg = sum / vec.size();
    auto [minIt, maxIt] = std::minmax_element(vec.begin(), vec.end());
    auto min = *minIt;
    auto max = *maxIt;
    std::sort(vec.begin(), vec.end());
    auto median = vec[vec.size() / 2];
    std::cout << "Min: " << min << " ns, Max: " << max << " ns, Avg: " << avg << " ns, Median: " << median << " ns" << std::endl;
}

void printIteration(size_t size, std::vector<double> &vec) {
    std::cout << "----------\n";
    std::cout << "size: " << size << " [B]\n";
    printTimes(vec);
}

void printInitialInfo(size_t cachelineSize, size_t comparisonIterations, size_t clflushIterations, OverrideMemType overrideMem, std::vector<size_t> &sizes, CachelineFlushOp clOp, bool alignMem, bool precise, bool constClSize) {
    std::cout << "clflush statistics\n";
    std::cout << "cachelineSize: " << cachelineSize << "\n";
    std::cout << "comparisonIterations [-i]: " << comparisonIterations << "\n";
    std::cout << "clflushIterations [-m]: " << clflushIterations << "\n";
    std::cout << "overrideMem [-f]: " << static_cast<uint32_t>(overrideMem) << "\n";
    std::cout << "cacheline flush type [-c]: " << static_cast<uint32_t>(clOp) << "\n";
    std::cout << "alignment [-a]: " << alignMem << "\n";
    std::cout << "precise [-p]: " << precise << "\n";
    std::cout << "constClSize [-t]: " << constClSize << "\n";
    std::cout << "sizes [-s]:";
    for (const auto &size : sizes) {
        std::cout << " " << size << ",";
    }
    std::cout << "\n";
}

void overrideMemoryOnes(std::vector<char> &mem) {
    for (auto &byte : mem) {
        byte = 1;
    }
}

void overrideMemoryRandom(std::vector<char> &mem) {
    srand(static_cast<unsigned>(time(NULL)));
    for (auto &byte : mem) {
        byte = rand();
    }
}

template <CachelineFlushOp Op, bool constCachelineSize>
void flushMemory(size_t cachelineSize, size_t size, char *ptr) {
    const auto lastPtr = ptr + size;
    while (ptr < lastPtr) {
        if constexpr (Op == CachelineFlushOp::MM_CLFLUSH) {
            _mm_clflush(ptr);
        } else if constexpr (Op == CachelineFlushOp::MM_CLFLUSHOPT) {
            _mm_clflushopt(ptr);
        } else {
            _mm_clwb(ptr);
        }
        if constexpr (constCachelineSize) {
            ptr += 64;
        } else {
            ptr += cachelineSize;
        }
    }
}

auto getTime() {
    _mm_mfence();
    _mm_lfence();
    return std::chrono::high_resolution_clock::now();
}

int main(int argc, char **argv) {
    size_t comparisonIterations = defaultComparisonIterations;
    size_t clflushIterations = defaultClflushIterations;
    OverrideMemType overrideMem = defaultOverrideMem;
    CachelineFlushOp clOp = defaultOpType;
    bool alignMem = defaultAlign;
    bool precise = defaultPrecise;
    bool constClSize = defaultClSize;

    std::vector<size_t> sizes{};

    for (size_t i = 1; i < argc; ++i) {
        if (!strcmp("-h", argv[i])) {
            printHelp();
            return 0;
        } else if (!strcmp("-s", argv[i])) {
            auto size = strtok(argv[++i], ",");
            while (size != NULL) {
                sizes.push_back(atoi(size));
                size = strtok(NULL, ",");
            }
        } else if (!strcmp("-f", argv[i])) {
            i++;
            if (!strcmp("NONE", argv[i])) {
                overrideMem = OverrideMemType::NONE;
            } else if (!strcmp("ONES", argv[i])) {
                overrideMem = OverrideMemType::ONES;
            } else if (!strcmp("RANDOM", argv[i])) {
                overrideMem = OverrideMemType::RANDOM;
            } else {
                std::cout << "Unknown override memory type \n";
                return 0;
            }
        } else if (!strcmp("-i", argv[i])) {
            comparisonIterations = atoi(argv[++i]);
        } else if (!strcmp("-p", argv[i])) {
            precise = atoi(argv[++i]);
        } else if (!strcmp("-m", argv[i])) {
            clflushIterations = atoi(argv[++i]);
        } else if (!strcmp("-a", argv[i])) {
            alignMem = atoi(argv[++i]);
        } else if (!strcmp("-t", argv[i])) {
            constClSize = atoi(argv[++i]);
        } else if (!strcmp("-c", argv[i])) {
            i++;
            if (!strcmp("MM_CLFLUSH", argv[i])) {
                clOp = CachelineFlushOp::MM_CLFLUSH;
            } else if (!strcmp("MM_CLFLUSHOPT", argv[i])) {
                clOp = CachelineFlushOp::MM_CLFLUSHOPT;
            } else if (!strcmp("MM_CLWB", argv[i])) {
                clOp = CachelineFlushOp::MM_CLWB;
            } else {
                std::cout << "Unknown cacheline flush type \n";
                return 0;
            }
        }
    }

    if (sizes.empty()) {
        for (const auto size : defaultSizes) {
            sizes.push_back(size);
        }
    }

    auto flushMemFunc = clOp == CachelineFlushOp::MM_CLFLUSH      ? &flushMemory<CachelineFlushOp::MM_CLFLUSH, false>
                        : clOp == CachelineFlushOp::MM_CLFLUSHOPT ? &flushMemory<CachelineFlushOp::MM_CLFLUSHOPT, false>
                                                                  : &flushMemory<CachelineFlushOp::MM_CLWB, false>;
    if (constClSize) {
        flushMemFunc = clOp == CachelineFlushOp::MM_CLFLUSH      ? &flushMemory<CachelineFlushOp::MM_CLFLUSH, true>
                       : clOp == CachelineFlushOp::MM_CLFLUSHOPT ? &flushMemory<CachelineFlushOp::MM_CLFLUSHOPT, true>
                                                                 : &flushMemory<CachelineFlushOp::MM_CLWB, true>;
    }

    const auto cachelineSize = getCachelineSize();
    if (cachelineSize == 0) {
        std::cout << "Query cacheline size failed\n";
        return 0;
    }

    printInitialInfo(cachelineSize, comparisonIterations, clflushIterations, overrideMem, sizes, clOp, alignMem, precise, constClSize);

    for (const auto size : sizes) {
        std::vector<double> times;
        times.reserve(comparisonIterations);

        auto sizeToAlloc = alignMem ? size + cachelineSize : size;
        std::vector<char> memory(sizeToAlloc);
        void *memPtr = memory.data();
        auto ptrToFlush = alignMem ? std::align(cachelineSize, size, memPtr, sizeToAlloc) : memPtr;

        for (size_t i = 0; i < comparisonIterations; ++i) {
            switch (overrideMem) {
            case OverrideMemType::ONES:
                overrideMemoryOnes(memory);
                break;
            case OverrideMemType::RANDOM:
                overrideMemoryRandom(memory);
                break;
            }

            auto start = getTime();

            for (size_t j = 0; j < clflushIterations; ++j) {
                flushMemFunc(cachelineSize, size, static_cast<char *>(ptrToFlush));
            }

            auto end = getTime();
            auto score = (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()) / static_cast<double>(clflushIterations);

            if (precise) {
                char *pointerToModify = static_cast<char *>(memPtr);

                auto start = getTime();

                for (size_t j = 0; j < clflushIterations; ++j) {
                    const auto lastPtr = pointerToModify + size;
                    while (pointerToModify < lastPtr) {
                        pointerToModify += cachelineSize;
                    }
                }

                auto end = getTime();

                if (pointerToModify < static_cast<char *>(memPtr) + size) {
                    std::cout << "Error in loop\n";
                    return 0;
                }

                auto preciseScore = (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()) / static_cast<double>(clflushIterations);
                score -= preciseScore;
            }

            times.push_back(score);
        }

        printIteration(size, times);
    }

    return 0;
}