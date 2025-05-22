#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

namespace spock {
    struct DescriptorAllocator {
        VkDescriptorType type;
        uint32_t         startSize;

        bool                          initialised = false;
        std::vector<VkDescriptorPool> pools;
        uint32_t                      currentPool = 0;
        uint32_t                      setsPerPool = 0;
        VkDescriptorPoolCreateFlags   flags       = 0;

        void                          init(VkDescriptorType type, uint32_t startSize);
        VkDescriptorPool              create_pool();
        void                          set_flags(VkDescriptorPoolCreateFlags flags);
        void                          clear_pools();
        void                          destroy_pools();
        VkDescriptorSet               allocate(VkDescriptorSetLayout layout);
    };
    struct GrowableDescriptorAllocator {
        struct PoolSizeRatio {
            VkDescriptorType type;
            float            ratio;
        };

        bool                          initialised = false;
        std::vector<PoolSizeRatio>    ratios;
        std::vector<VkDescriptorPool> pools;
        uint32_t                      currentPool = 0;
        uint32_t                      setsPerPool = 0;
        VkDescriptorPoolCreateFlags   flags       = 0;

        void                          init(std::initializer_list<PoolSizeRatio> _ratios, uint32_t maxSets);
        VkDescriptorPool              create_pool();
        void                          set_flags(VkDescriptorPoolCreateFlags flags);
        void                          clear_pools();
        void                          destroy_pools();
        VkDescriptorSet               allocate(VkDescriptorSetLayout layout);
    };
}
