#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>

namespace spock {
    struct DescriptorAllocator {
        struct PoolSizeRatio {
            VkDescriptorType type;
            uint32_t         ratio;
        };

        bool                          initialised = false;
        std::vector<PoolSizeRatio>    ratios;
        std::vector<VkDescriptorPool> pools;
        uint32_t                      currentPool = 0;
        uint32_t                      setsPerPool = 0;
        VkDescriptorPoolCreateFlags   flags       = 0;

        void                          init(std::initializer_list<PoolSizeRatio> _ratios, uint32_t maxSets, VkDescriptorPoolCreateFlags _flags);
        VkDescriptorPool              create_pool();
        void                          clear_pools();
        void                          destroy_pools();
        VkDescriptorSet               allocate(VkDescriptorSetLayout layout);
    };
}
