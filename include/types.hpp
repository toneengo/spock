#pragma once
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>
#include <vector>
namespace spock {
    struct Binding {
        uint32_t                 binding;
        VkDescriptorType         type;
        uint32_t                 count;
        VkDescriptorBindingFlags flags = 0;
    };

    struct Image {
        VkImage       image       = VK_NULL_HANDLE;
        VkImageView   imageView   = VK_NULL_HANDLE;
        VmaAllocation allocation  = VK_NULL_HANDLE;
        VkExtent3D    imageExtent = {};
        VkFormat      imageFormat = VK_FORMAT_UNDEFINED;
        uint32_t      index       = 0;
    };

    struct Buffer {
        VkBuffer          buffer;
        VmaAllocation     allocation;
        VmaAllocationInfo info;
    };

    //since we use dynamic rendering, VkFramebuffer isn't used, instead we have a collection of color/depth/stencil attchments
    //#TODO: get rid of this plz famebuffer is a bad name
    struct Framebuffer {
        std::vector<Image> color;
        Image              depth;
        Image              stencil;
        uint32_t           viewMask;
    };
}
