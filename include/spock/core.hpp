#pragma once
#include <vulkan/vulkan_core.h>
#undef VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "shader.hpp"

namespace spock {
    void  clean_init();

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

    void                  update_descriptor_sets(std::initializer_list<ImageWrite> imageWrites, std::initializer_list<BufferWrite> bufferWrites);

    void                  init();
    
    void                  cleanup();
    VkDescriptorSetLayout create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags = 0);
    VkPipelineLayout      create_pipeline_layout(std::initializer_list<VkDescriptorSetLayout> dsLayouts, std::initializer_list<VkPushConstantRange> psRanges);

    Image                 create_image_from_pixels(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false);

    Image                 create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false);
    inline Image          create_image(VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false)
        { return create_image(VkExtent3D{.width = size.width, .height = size.height, .depth = 1}, format, usage, viewType, mipmapped); }

    Image                 create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false);
    Image                 create_image(const char* fileName, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false);

    Image                 create_texture(const char* fileName, uint32_t index, VkDescriptorSet descriptorSet, uint32_t binding, VkSampler sampler, VkImageUsageFlags usage, VkImageViewType viewType, bool mipmapped = false);
    void                  create_texture(Image& image, uint32_t index, VkDescriptorSet descriptorSet, uint32_t binding, VkSampler sampler);

    void                  destroy_image(Image image);

    Buffer                create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

    // ONLY use for gpu-only buffers at initialisation
    void copy_to_buffer(VkBuffer buffer, void* src, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
    void                  destroy_buffer(Buffer buffer);

    void                  create_swapchain(uint32_t width, uint32_t height);

    VkCommandBuffer       get_immediate_command_buffer();
    void                  begin_immediate_command();
    void                  end_immediate_command();

}
