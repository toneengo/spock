#include "spock/descriptor.hpp"
#include "spock/internal.hpp"
#include "spock/util.hpp"

using namespace spock;

void DescriptorAllocator::init(VkDescriptorType t, uint32_t sz) {
    type = t;
    startSize = sz;
    setsPerPool = 1; //grow it next allocation
    pools.push_back(create_pool());
    initialised = true;
}

VkDescriptorPool DescriptorAllocator::create_pool() {
    VkDescriptorPoolSize sz = {.type = type, .descriptorCount = uint32_t(startSize * setsPerPool)};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = flags;
    pool_info.maxSets                    = 64;
    pool_info.poolSizeCount              = 1;
    pool_info.pPoolSizes                 = &sz;

    VkDescriptorPool pool;
    VkResult r = vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &pool);
    assert(r == VK_SUCCESS);
    return pool;
}

void DescriptorAllocator::clear_pools() {
    for (auto p : pools) {
        vkResetDescriptorPool(ctx.device, p, 0);
    }
    currentPool = 0;
}

void DescriptorAllocator::destroy_pools() {
    for (auto p : pools) {
        vkDestroyDescriptorPool(ctx.device, p, nullptr);
    }
    currentPool = 0;
}

void DescriptorAllocator::set_flags(VkDescriptorPoolCreateFlags _flags) {
    flags = _flags;
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout) {
    assert(initialised);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext                       = VK_NULL_HANDLE;
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = pools[currentPool];
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &layout;

    VkDescriptorSet ds;
    VkResult        result = vkAllocateDescriptorSets(ctx.device, &allocInfo, &ds);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        currentPool++;
        if (currentPool == pools.size()) {
            pools.push_back(create_pool());
            setsPerPool *= 2;
        }
        allocInfo.descriptorPool = pools[currentPool];
        VK_CHECK(vkAllocateDescriptorSets(ctx.device, &allocInfo, &ds));
    }
    return ds;
}
