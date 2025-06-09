#pragma once
#include <vulkan/vulkan_core.h>
#undef VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "shader.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace spock {
    void  clean_init();

    //command buffer for immediate commands
    inline VkCommandPool   immCommandPool;
    inline VkCommandBuffer immCommandBuffer;
    inline VkFence         immCommandFence;

    struct ImageWrite {
        VkDescriptorSet  descriptorSet;
        uint32_t         binding;
        VkDescriptorType descriptorType;
        VkSampler        sampler;
        VkImageView      imageView;
        VkImageLayout    imageLayout;
        uint32_t         index = 0;
        uint32_t         count = 1;
    };
    struct BufferWrite {
        VkDescriptorSet  descriptorSet;
        uint32_t         binding;
        VkDescriptorType descriptorType;
        VkBuffer         buffer;
        VkDeviceSize     offset;
        VkDeviceSize     range;
        uint32_t         index = 0;
        uint32_t         count = 1;
    };

    void                  write_descriptor_sets(std::initializer_list<ImageWrite> imageWrites, std::initializer_list<BufferWrite> bufferWrites);

    void                  init(SDL_Window* window);
    void                  process_SDL_event(const SDL_Event& event);
    
    void                  cleanup();
    VkDescriptorSetLayout create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags = 0);
    VkPipelineLayout      create_pipeline_layout(std::initializer_list<VkDescriptorSetLayout> dsLayouts, std::initializer_list<VkPushConstantRange> psRanges);

    Image                 create_image(VkExtent3D extent, VkImageType type, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels = 1);
    Image                 create_image(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels = 1);
    Image                 create_image(const char* fileName, VkImageUsageFlags usage, uint32_t mipLevels = 1);
    VkImageView           create_image_view(const spock::Image& image, VkImageViewType viewType, VkExtent3D extent, uint32_t baseMipLevel = 0, uint32_t mipLevels = 1, uint32_t baseArrayLayer = 0);
    inline VkImageView    create_image_view(const spock::Image& image, VkImageViewType viewType, VkExtent2D extent, uint32_t baseMipLevel = 0, uint32_t mipLevels = 1, uint32_t baseArrayLayer = 0)
    { create_image_view(image, viewType, {extent.width, extent.height, 1}, baseMipLevel, mipLevels, baseArrayLayer); }
    // global imageview
    spock::Image          create_image_and_view(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, uint32_t mipLevels = 1);
    inline Image          create_image_and_view(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, uint32_t mipLevels = 1)
    { return create_image_and_view({extent.width, extent.height, 1}, format, usage, viewType, mipLevels); }
    void                  destroy_image_view(VkImageView view);

    VkCommandPool         create_command_pool(VkCommandPoolCreateFlags flags);
    VkCommandBuffer       create_command_buffer(VkCommandPool pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VkFence create_fence(VkFenceCreateFlagBits flags = VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphore create_semaphore();

    void                  destroy_image(Image image);

    Buffer                create_buffer(size_t allocSize, VkBufferUsageFlags usage, uint32_t requiredFlags);
    Buffer                create_buffer(void* data, size_t allocSize, VkBufferUsageFlags usage, uint32_t requiredFlags);

    // ONLY use for gpu-only buffers at initialisation
    void copy_to_buffer(VkBuffer buffer, void* src, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
    void                  destroy_buffer(Buffer buffer);

    void                  destroy_swapchain();
    void                  create_swapchain(uint32_t width, uint32_t height);

    void                  begin_immediate_command();
    void                  end_immediate_command();

}
