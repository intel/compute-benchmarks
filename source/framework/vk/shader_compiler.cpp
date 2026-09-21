/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shader_compiler.h"

#include "framework/utility/error.h"
#include "framework/utility/file_helper.h"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <memory>

namespace VK {

namespace {

void initializeGlslang() {
    static const bool initialized = glslang_initialize_process() != 0;
    FATAL_ERROR_IF(!initialized, "glslang_initialize_process failed");
}

std::string definesToPreamble(std::span<const ShaderDefine> defines) {
    std::string preamble;
    for (const auto &[name, value] : defines) {
        preamble += "#define " + name + " " + value + "\n";
    }
    return preamble;
}

struct ShaderDeleter {
    void operator()(glslang_shader_t *shader) const { glslang_shader_delete(shader); }
};
struct ProgramDeleter {
    void operator()(glslang_program_t *program) const { glslang_program_delete(program); }
};

} // namespace

TestResult ShaderCompiler::compileComputeShaderToSpirv(const std::string &shaderFileName,
                                                       std::span<const ShaderDefine> defines,
                                                       std::vector<uint32_t> &outSpirv) {
    const auto sourceBytes = FileHelper::loadBinaryFile(shaderFileName);
    if (sourceBytes.empty()) {
        return TestResult::KernelNotFound;
    }
    const std::string source(sourceBytes.begin(), sourceBytes.end());
    const std::string preamble = definesToPreamble(defines);

    initializeGlslang();

    // Same settings as glslc --target-env=vulkan1.2 -O0
    glslang_input_t input{};
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = GLSLANG_STAGE_COMPUTE;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_2;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_5;
    input.code = source.c_str();
    input.default_version = 110;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.messages = static_cast<glslang_messages_t>(GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    input.resource = glslang_default_resource();

    const std::unique_ptr<glslang_shader_t, ShaderDeleter> shader(glslang_shader_create(&input));
    FATAL_ERROR_IF(shader == nullptr, "glslang_shader_create failed");
    glslang_shader_set_preamble(shader.get(), preamble.c_str());

    if (!glslang_shader_preprocess(shader.get(), &input) || !glslang_shader_parse(shader.get(), &input)) {
        printMessageLine("ERROR", "failed to compile ", shaderFileName, ":\n", glslang_shader_get_info_log(shader.get()));
        return TestResult::KernelBuildError;
    }

    const std::unique_ptr<glslang_program_t, ProgramDeleter> program(glslang_program_create());
    FATAL_ERROR_IF(program == nullptr, "glslang_program_create failed");
    glslang_program_add_shader(program.get(), shader.get());
    if (!glslang_program_link(program.get(), input.messages)) {
        printMessageLine("ERROR", "failed to link ", shaderFileName, ":\n", glslang_program_get_info_log(program.get()));
        return TestResult::KernelBuildError;
    }

    glslang_spv_options_t spvOptions{};
    spvOptions.disable_optimizer = true;
    glslang_program_SPIRV_generate_with_options(program.get(), GLSLANG_STAGE_COMPUTE, &spvOptions);
    if (const char *const messages = glslang_program_SPIRV_get_messages(program.get()); messages != nullptr && *messages != '\0') {
        printMessageLine("WARNING", "SPIR-V generation for ", shaderFileName, ":\n", messages);
    }

    outSpirv.resize(glslang_program_SPIRV_get_size(program.get()));
    glslang_program_SPIRV_get(program.get(), outSpirv.data());
    return TestResult::Success;
}

} // namespace VK
