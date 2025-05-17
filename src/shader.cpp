#include <cstdint>
#include <fstream>
#include <set>
#include <algorithm>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include "spock/internal.hpp"

#include "spock/shader.hpp"

void error_exit();
//Shaders

//from glslang repo
// If no path markers, return current working directory.
// Otherwise, strip file name and return path leading up to it.
std::string getDirectory(const std::string path) {
    size_t last = path.find_last_of("/\\");
    return last == std::string::npos ? "." : path.substr(0, last);
}

class DirStackFileIncluder : public glslang::TShader::Includer {
  public:
    DirStackFileIncluder() : externalLocalDirectoryCount(0) {}

    virtual IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override {
        return readLocalPath(headerName, includerName, (int)inclusionDepth);
    }

    virtual IncludeResult* includeSystem(const char* headerName, const char* /*includerName*/, size_t /*inclusionDepth*/) override {
        return readSystemPath(headerName);
    }

    // Externally set directories. E.g., from a command-line -I<dir>.
    //  - Most-recently pushed are checked first.
    //  - All these are checked after the parse-time stack of local directories
    //    is checked.
    //  - This only applies to the "local" form of #include.
    //  - Makes its own copy of the path.
    virtual void pushExternalLocalDirectory(const std::string& dir) {
        directoryStack.push_back(dir);
        externalLocalDirectoryCount = (int)directoryStack.size();
    }

    virtual void releaseInclude(IncludeResult* result) override {
        if (result != nullptr) {
            delete[] static_cast<tUserDataElement*>(result->userData);
            delete result;
        }
    }

    virtual std::set<std::string> getIncludedFiles() {
        return includedFiles;
    }

    virtual ~DirStackFileIncluder() override {}

  protected:
    typedef char             tUserDataElement;
    std::vector<std::string> directoryStack;
    int                      externalLocalDirectoryCount;
    std::set<std::string>    includedFiles;

    // Search for a valid "local" path based on combining the stack of include
    // directories and the nominal name of the header.
    virtual IncludeResult* readLocalPath(const char* headerName, const char* includerName, int depth) {
        // Discard popped include directories, and
        // initialize when at parse-time first level.
        directoryStack.resize(depth + externalLocalDirectoryCount);
        if (depth == 1)
            directoryStack.back() = getDirectory(includerName);

        // Find a directory that works, using a reverse search of the include stack.
        for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); ++it) {
            std::string path = *it + '/' + headerName;
            std::replace(path.begin(), path.end(), '\\', '/');
            std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);
            if (file) {
                directoryStack.push_back(getDirectory(path));
                includedFiles.insert(path);
                return newIncludeResult(path, file, (int)file.tellg());
            }
        }

        return nullptr;
    }

    // Search for a valid <system> path.
    // Not implemented yet; returning nullptr signals failure to find.
    virtual IncludeResult* readSystemPath(const char* /*headerName*/) const {
        return nullptr;
    }

    // Do actual reading of the file, filling in a new include result.
    virtual IncludeResult* newIncludeResult(const std::string& path, std::ifstream& file, int length) const {
        char* content = new tUserDataElement[length];
        file.seekg(0, file.beg);
        file.read(content, length);
        return new IncludeResult(path, content, length, content);
    }

};


std::vector<uint32_t> spock::glsl_to_spirv(const char* const* shaderSource, EShLanguage stage, const char* filePath) {
    glslang::InitializeProcess();
    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory(getDirectory(filePath));

    glslang::TShader shader(stage);
    shader.setDebugInfo(true);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    shader.setStrings(shaderSource, 1);
    if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault, includer)) {
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        return {};
    }

    const char* const* ptr;
    int                n = 0;
    shader.getStrings(ptr, n);

    glslang::TProgram program;
    program.addShader(&shader);
    program.link(EShMsgDefault);

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    glslang::FinalizeProcess();

    return spirv;
}

std::vector<VkShaderModule> shaderModulesToClean;
VkShaderModule              spock::create_shader_module(const char* filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        printf("Couldn't open file %s\n", filePath);
        error_exit();
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
        error_exit();
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
        error_exit();
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
