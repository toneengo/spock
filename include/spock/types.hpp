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
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlagBits2 currentStage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 currentAccess = VK_ACCESS_2_NONE;
    };

    struct Buffer {
        VkBuffer          buffer;
        VmaAllocation     allocation;
        VmaAllocationInfo info;
        VkDeviceSize      size = 0;
    };
}
