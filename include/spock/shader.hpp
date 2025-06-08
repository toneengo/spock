#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
namespace spock {
    VkShaderModule create_shader_module(size_t bufsize, uint32_t* spirv);
    VkShaderModule create_shader_module(const char* filePath);
    void           destroy_shader_module(VkShaderModule module);
    void           clean_shader_modules();
}
