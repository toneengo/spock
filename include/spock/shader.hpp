#pragma once
#include <vulkan/vulkan_core.h>
#include <glslang/Public/ShaderLang.h>
#include <vector>
namespace spock {
    std::vector<uint32_t> glsl_to_spirv(const char* const* shaderSource, EShLanguage stage, const char* filePath);
    VkShaderModule create_shader_module(size_t bufsize, uint32_t* spirv);
    VkShaderModule create_shader_module(const char* filePath);
    void           destroy_shader_module(VkShaderModule module);
    void           clean_shader_modules();
}
