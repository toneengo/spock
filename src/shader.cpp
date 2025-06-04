#include <cstdint>
#include <fstream>
#include <set>
#include <algorithm>
#include "spock/internal.hpp"

#include "spock/shader.hpp"
//Shaders

std::vector<VkShaderModule> shaderModulesToClean;
VkShaderModule              spock::create_shader_module(const char* filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        printf("Couldn't open file %s\n", filePath);
        assert(false);
        //return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string buffer(size, ' ');
    file.seekg(0);
    file.read(&buffer[0], size);
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext                    = nullptr;
    createInfo.codeSize                 = buffer.size();
    createInfo.pCode                    = (uint32_t*)buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        assert(false);
    }
    shaderModulesToClean.push_back(shaderModule);

    return shaderModule;
}

VkShaderModule spock::create_shader_module(size_t bufsize, uint32_t* spirv)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext                    = nullptr;
    createInfo.codeSize                 = bufsize;
    createInfo.pCode                    = spirv;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(ctx.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        assert(false);
    }
    shaderModulesToClean.push_back(shaderModule);

    return shaderModule;
}

void spock::clean_shader_modules() {
    for (const auto& s : shaderModulesToClean) {
        vkDestroyShaderModule(ctx.device, s, nullptr);
    }
    shaderModulesToClean.clear();
}

void spock::destroy_shader_module(VkShaderModule module) {
    vkDestroyShaderModule(ctx.device, module, nullptr);
}
