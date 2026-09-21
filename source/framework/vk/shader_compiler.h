/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_result.h"

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace VK {

// A single preprocessor define handed to the shader compiler, equivalent to the -Dname=value
// argument passed to glslc.
using ShaderDefine = std::pair<std::string, std::string>;

struct ShaderCompiler {
    // Compiles a GLSL compute shader into SPIR-V words, the same way as
    // glslc --target-env=vulkan1.2 -O0
    //
    // Returns:
    //   KernelNotFound   - the shader source is not in the working directory,
    //   KernelBuildError - the shader did not compile.
    static TestResult compileComputeShaderToSpirv(const std::string &shaderFileName,
                                                  std::span<const ShaderDefine> defines,
                                                  std::vector<uint32_t> &outSpirv);
};

} // namespace VK
