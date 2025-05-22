//here, the vulkan data (RenderContext), is a singleton object that is accessible via #include "interal.hpp"
#pragma once
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"
#include "types.hpp"
#include "destroy.hpp"
#include "descriptor.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace spock {
    constexpr uint32_t FRAME_OVERLAP = 1;
    struct FrameContext {
        VkCommandPool               commandPool;
        VkCommandBuffer             commandBuffer;
        VkSemaphore                 swapchainSemaphore, renderSemaphore;
        VkFence                     renderFence;
        spock::DestroyQueue        destroyQueue;
        spock::DescriptorAllocator descriptorAllocator;
    };

    inline struct RenderContext {
        bool                        initialised = false;
        VkDevice                    device;
        VkInstance                  instance;
        VkDebugUtilsMessengerEXT    debugMessenger;
        VkSurfaceKHR                surface;
        VkPhysicalDevice            physicalDevice;
        VkQueue                     graphicsQueue;
        uint32_t                    graphicsQueueFamily;
        VmaAllocator                allocator;

        FrameContext                frames[FRAME_OVERLAP];

        uint32_t                    frameIdx = 0;

        //command buffer for immediate commands
        VkCommandPool   immCommandPool;
        VkCommandBuffer immCommandBuffer;
        VkFence         immCommandFence;

        SDL_Window*     window;
        VkExtent2D      windowExtent = {800, 600};
        VkExtent3D      screenExtent; //desktop resolution
        glm::vec2       renderScale = {1.f, 1.f};

        struct Swapchain {
            VkSwapchainKHR           swapchain;
            std::vector<spock::Image>images;
            VkExtent2D               extent;
            VkFormat imageFormat;
        };

        //settable by the "user"
        /*
        VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
        VkSampler currentSampler = VK_NULL_HANDLE;
        uint32_t textureDescriptorSetBinding = 0;
        uint32_t textureCount = 0;
        */

        Swapchain            swapchain;
        spock::DestroyQueue destroyQueue;

        bool resizeRequested = true;
    } ctx;

    inline uint32_t         get_frame_number() {
        return ctx.frameIdx % FRAME_OVERLAP;
    }
    inline FrameContext&         get_frame() {
        return ctx.frames[ctx.frameIdx % FRAME_OVERLAP];
    }
    inline void         finish_frame() {
        ctx.frameIdx++;
    }
    inline DestroyQueue destroyQueue;

#ifdef DBG
#define QUEUE_DESTROY_OBJ(x)                                                                                                                                                       \
    do {                                                                                                                                                                           \
        spock::Object o(x);                                                                                                                                                       \
        o.lineNumber = __LINE__;                                                                                                                                                   \
        o.fileName   = __FILE__;                                                                                                                                                   \
        spock::destroyQueue.push(o);                                                                                                                                              \
    } while (0);
#else
#define QUEUE_DESTROY_OBJ(x) spock::destroyQueue.push(x);
#endif

}
