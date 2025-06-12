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
    inline struct RenderContext {
        bool                        initialised = false;
        VkDevice                    device;
        VkInstance                  instance;
        VkDebugUtilsMessengerEXT    debugMessenger;
        VkSurfaceKHR                surface;
        VkPhysicalDevice            physicalDevice;
        VkQueue                     graphicsQueue;
        uint32_t                    graphicsQueueFamily;

        VkQueue                     computeQueue;
        uint32_t                    computeQueueFamily;

        VkQueue                     transferQueue;
        uint32_t                    transferQueueFamily;
        VmaAllocator                allocator;

        SDL_Window*     window;

        spock::DestroyQueue destroyQueue;
    } ctx;

#ifdef DBG
#define QUEUE_DESTROY_OBJ(x)                                                                                                                                                       \
    do {                                                                                                                                                                           \
        spock::ctx.destroyQueue.push(x);                                                                                                                                              \
        spock::ctx.destroyQueue.queue.back().lineNumber = __LINE__;                                                                                                                                                   \
        spock::ctx.destroyQueue.queue.back().fileName   = __FILE__;                                                                                                                                                   \
    } while (0);
#else
#define QUEUE_DESTROY_OBJ(x) spock::destroyQueue.push(x);
#endif

}
