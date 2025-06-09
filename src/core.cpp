#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "vk_mem_alloc.h"

#include "VkBootstrap.h"
#include <SDL3/SDL_vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "spock/util.hpp"
#include "spock/core.hpp"
#include "spock/internal.hpp"
#include "spock/destroy.hpp"
#include "spock/shader.hpp"
#include <iostream>

#ifdef DBG
const bool gEnableValidationLayers = true;
#else
const bool gEnableValidationLayers = false;
#endif

using namespace spock;

void spock::destroy_swapchain() {
    vkDestroySwapchainKHR(ctx.device, ctx.swapchain.swapchain, nullptr);
    for (const auto& v : ctx.swapchain.views) {
        vkDestroyImageView(ctx.device, v, nullptr);
    }

    ctx.swapchain.images.clear();
}

void spock::process_SDL_event(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
            destroy_swapchain();
            create_swapchain(event.window.data1, event.window.data2);
            break;
        default:
            break;
    }
}

static void init_device(SDL_Window* window) {
    ctx.window = window;

    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("vulkan app")
#ifdef DBG
                        .request_validation_layers(gEnableValidationLayers)
                        //.enable_layer("VK_LAYER_KHRONOS_synchronization2")
#endif
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build();

    if (!inst_ret)
    {
        std::cerr << "Failed to create Vulkan Instance. Error: " << inst_ret.error().message() << '\n';
    }

    vkb::Instance vkb_inst = inst_ret.value();
    ctx.instance           = vkb_inst.instance;
    ctx.debugMessenger     = vkb_inst.debug_messenger;

    if (!SDL_Vulkan_CreateSurface(ctx.window, ctx.instance, nullptr, &ctx.surface))
    {
        printf("Couldnt create surface: %s\n", SDL_GetError());
    }

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress                           = true;
    features12.shaderOutputViewportIndex                     = true;
    features12.shaderOutputLayer                     = true;
    features12.descriptorIndexing                            = true;
    features12.runtimeDescriptorArray                        = true;
    features12.shaderUniformBufferArrayNonUniformIndexing    = true;
    features12.shaderStorageImageArrayNonUniformIndexing     = true;
    features12.shaderSampledImageArrayNonUniformIndexing     = true;
    features12.shaderStorageImageArrayNonUniformIndexing     = true;
    features12.descriptorBindingUniformBufferUpdateAfterBind = true;
    features12.descriptorBindingSampledImageUpdateAfterBind  = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind = true;
    features12.descriptorBindingStorageImageUpdateAfterBind  = true;

    VkPhysicalDeviceVulkan11Features features11{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    features11.multiview = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto phys_ret = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .set_required_features_11(features11)
        .set_surface(ctx.surface)
        .select();

    if (!phys_ret)
    {
        std::cerr << "Failed to select Vulkan Physical Device. Error: " << phys_ret.error().message() << '\n';
    }
    
    vkb::PhysicalDevice physical_device = phys_ret.value();

    vkb::DeviceBuilder device_builder{physical_device};
    vkb::Device        vkb_device = device_builder.build().value();
    ctx.device                    = vkb_device.device;
    ctx.graphicsQueue             = vkb_device.get_queue(vkb::QueueType::graphics).value();
    ctx.graphicsQueueFamily       = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    ctx.physicalDevice            = physical_device.physical_device;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = ctx.physicalDevice;
    allocatorInfo.device                 = ctx.device;
    allocatorInfo.instance               = ctx.instance;
    allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &ctx.allocator);
    QUEUE_DESTROY_OBJ(ctx.allocator);
}

static void init_swapchain() {
    //initialize swapchain
    // no way this should change right??
    create_swapchain(ctx.windowExtent.width, ctx.windowExtent.height);
    ctx.swapchain.imageCount = ctx.swapchain.images.size();
}

void spock::init(SDL_Window* window)
{
    init_device(window);
    init_swapchain();

    immCommandFence = create_fence(VK_FENCE_CREATE_SIGNALED_BIT);
    immCommandPool = create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    immCommandBuffer = create_command_buffer(immCommandPool);

    QUEUE_DESTROY_OBJ(immCommandPool);
    QUEUE_DESTROY_OBJ(immCommandFence);

    ctx.initialised = true;
}

void spock::clean_init() {
    clean_shader_modules();
}

VkDescriptorSetLayout spock::create_descriptor_set_layout(std::initializer_list<Binding> _bindings, VkShaderStageFlags shaderStages, VkDescriptorSetLayoutCreateFlags flags) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBindingFlags> bindingFlags;
    for (auto& b : _bindings) {
        bindings.push_back({.binding = b.binding, .descriptorType = b.type, .descriptorCount = b.count, .stageFlags = shaderStages});
        bindingFlags.push_back(b.flags);
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    binding_info.bindingCount = bindingFlags.size();
    binding_info.pBindingFlags = bindingFlags.data();

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext                           = &binding_info;
    info.pBindings                       = bindings.data();
    info.bindingCount                    = bindings.size();
    info.flags                           = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &info, nullptr, &set));
    QUEUE_DESTROY_OBJ(set);

    return set;
}

VkPipelineLayout      spock::create_pipeline_layout(std::initializer_list<VkDescriptorSetLayout> dsLayouts, std::initializer_list<VkPushConstantRange> psRanges)
{
    VkPipelineLayout layout;
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext                  = nullptr;
    layoutInfo.pSetLayouts            = std::data(dsLayouts);
    layoutInfo.setLayoutCount         = dsLayouts.size();
    layoutInfo.pPushConstantRanges    = std::data(psRanges);
    layoutInfo.pushConstantRangeCount = psRanges.size();
    VK_CHECK(vkCreatePipelineLayout(spock::ctx.device, &layoutInfo, nullptr, &layout));
    return layout;
}
union DescriptorWriteInfo {
    VkDescriptorBufferInfo bufInfo;
    VkDescriptorImageInfo  imgInfo;
};
void spock::write_descriptor_sets(std::initializer_list<ImageWrite> imageWrites, std::initializer_list<BufferWrite> bufferWrites) {
    static VkWriteDescriptorSet writeSets[32]; //max 32 writes for now
    static DescriptorWriteInfo  writeInfos[32];

    int                  i = 0;
    for (auto& w : imageWrites) {
#ifdef DBG
        assert(i < 32);
#endif
        if (w.imageView == VK_NULL_HANDLE) continue;
        writeInfos[i].imgInfo.sampler     = w.sampler;
        writeInfos[i].imgInfo.imageView   = w.imageView;
        writeInfos[i].imgInfo.imageLayout = w.imageLayout;

        writeSets[i] = VkWriteDescriptorSet{
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = w.descriptorSet,
            .dstBinding       = w.binding,
            .dstArrayElement  = w.index,
            .descriptorCount  = w.count,
            .descriptorType   = w.descriptorType,
            .pImageInfo       = &writeInfos[i].imgInfo,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr //unused for now
        };

        i++;
    }
    for (auto& w : bufferWrites) {
#ifdef DBG
        assert(i < 32);
#endif
        if (w.range == 0 || w.buffer == VK_NULL_HANDLE) continue;

        writeInfos[i].bufInfo.buffer = w.buffer;
        writeInfos[i].bufInfo.range  = w.range;
        writeInfos[i].bufInfo.offset = w.offset;

        writeSets[i] = VkWriteDescriptorSet{
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = w.descriptorSet,
            .dstBinding       = w.binding,
            .dstArrayElement  = w.index,
            .descriptorCount  = w.count,
            .descriptorType   = w.descriptorType,
            .pImageInfo       = nullptr,
            .pBufferInfo      = &writeInfos[i].bufInfo,
            .pTexelBufferView = nullptr //unused for now
        };

        i++;
    }
    vkUpdateDescriptorSets(ctx.device, i, writeSets, 0, nullptr);
}

void spock::begin_immediate_command() {
    VK_CHECK(vkResetFences(ctx.device, 1, &immCommandFence));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer, 0));
    begin_command_buffer(immCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void spock::end_immediate_command() {
    VK_CHECK(vkEndCommandBuffer(immCommandBuffer));
    submit_command_buffer(
        {immCommandBuffer}, {}, {}, immCommandFence
    );
    VK_CHECK(vkWaitForFences(ctx.device, 1, &immCommandFence, true, 9999999999));
}

Buffer spock::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    // allocate buffer
    Buffer buffer;
    buffer.size = allocSize;

    if (allocSize == 0) return buffer;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext              = nullptr;
    bufferInfo.size               = allocSize;
    bufferInfo.usage              = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage                   = memoryUsage;
    vmaallocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VK_CHECK(vmaCreateBuffer(ctx.allocator, &bufferInfo, &vmaallocInfo, &buffer.buffer, &buffer.allocation, &buffer.info));
    return buffer;
}

Buffer spock::create_buffer(void* data, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    Buffer buffer = create_buffer(allocSize, usage, memoryUsage);
    imm_copy_to_buffer(buffer.buffer, data, allocSize);
    return buffer;
}

void spock::copy_to_buffer(VkBuffer buffer, void* src, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) {

    Buffer uploadbuffer = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data = uploadbuffer.info.pMappedData;
    memcpy(uploadbuffer.info.pMappedData, src, size);
    begin_immediate_command();

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = srcOffset;
    bufferCopy.dstOffset = dstOffset;
    bufferCopy.size = size;

    vkCmdCopyBuffer(immCommandBuffer, uploadbuffer.buffer, buffer, 1, &bufferCopy);
    end_immediate_command();

    vmaDestroyBuffer(ctx.allocator, uploadbuffer.buffer, uploadbuffer.allocation);
}

Image spock::create_image(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels) {
    size_t data_size    = extent.depth * extent.width * extent.height * 4;
    Buffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    memcpy(uploadbuffer.info.pMappedData, data, data_size);

    Image new_image = create_image(extent, VK_IMAGE_TYPE_2D, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipLevels);
    begin_immediate_command();

    image_barrier(immCommandBuffer, new_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, VK_ACCESS_2_MEMORY_WRITE_BIT_KHR);

    VkBufferImageCopy copyRegion = {};

    copyRegion.bufferOffset      = 0;
    copyRegion.bufferRowLength   = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;
    copyRegion.imageExtent                     = extent;

    // copy the buffer into the image
    vkCmdCopyBufferToImage(immCommandBuffer, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    image_barrier(immCommandBuffer, new_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR,
            VK_ACCESS_2_SHADER_READ_BIT_KHR);

    end_immediate_command();

    vmaDestroyBuffer(ctx.allocator, uploadbuffer.buffer, uploadbuffer.allocation);

    return new_image;
}

Image spock::create_image(const char* fileName, VkImageUsageFlags usage, uint32_t mipLevels)
{
    VkExtent3D     extent{};
    int            width = 0, height = 0;
    unsigned char* pixels = nullptr;
    bool empty = false;
    pixels = stbi_load(fileName, &width, &height, nullptr, 4);

    //create empty pixel image if couldn't load
    if (width == 0 || height == 0)
    {
        extent.width          = 1;
        extent.height         = 1;
        extent.depth          = 1;
        pixels = new unsigned char[4];
        memset(pixels, 0, sizeof(unsigned char)*4);
        empty = true;
    }
    else
    {
        extent.width          = width;
        extent.height         = height;
        extent.depth          = 1;
    }

    auto image = create_image(pixels, extent, VK_FORMAT_R8G8B8A8_UNORM, usage, mipLevels);
    if (empty) free(pixels);
    else stbi_image_free(pixels);
    return image;
}

spock::Image spock::create_image(VkExtent3D extent, VkImageType type, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels) {
    spock::Image image;
    image.format = format;
    image.extent = extent;

    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = type,
        .format = format,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = type == VK_IMAGE_TYPE_3D ? 1 : type == VK_IMAGE_TYPE_2D ? extent.depth : extent.height,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
    };

    switch(type)
    {
        case VK_IMAGE_TYPE_2D:
        info.extent.depth = 1;
        break;
        case VK_IMAGE_TYPE_1D:
        info.extent.height = 1;
        info.extent.depth = 1;
        break;
        default:
        break;
    }

    VmaAllocationCreateInfo alloc = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VK_CHECK(vmaCreateImage(ctx.allocator, &info, &alloc, &image.image, &image.allocation, nullptr));

    return image;
}

VkImageView spock::create_image_view(const spock::Image& image, VkImageViewType viewType, VkExtent3D extent, uint32_t baseMipLevel, uint32_t mipLevels, uint32_t baseArrayLayer)
{
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = viewType,
        .format = image.format,
        .subresourceRange = {
            .baseMipLevel = baseMipLevel,
            .levelCount = mipLevels,
            .baseArrayLayer = baseArrayLayer,
        }
    };

    //may need more
    switch(info.format)
    {
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D16_UNORM:
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
        default:
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }

    switch(viewType)
    {
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        info.subresourceRange.layerCount = extent.depth;
        break;
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
        info.subresourceRange.layerCount = extent.height;
        break;
        default:
        info.subresourceRange.layerCount = 1;
        break;
    }

    VkImageView view;
    VK_CHECK(vkCreateImageView(ctx.device, &info, nullptr, &view));

    return view;
}

spock::Image spock::create_image_and_view(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkImageViewType viewType, uint32_t mipLevels)
{
    VkImageType type;
    uint32_t layerCount;

    switch (viewType)
    {
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
        case VK_IMAGE_VIEW_TYPE_1D:
        type = VK_IMAGE_TYPE_1D;
        break;

        case VK_IMAGE_VIEW_TYPE_3D:
        type = VK_IMAGE_TYPE_3D;
        break;

        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        case VK_IMAGE_VIEW_TYPE_2D:
        type = VK_IMAGE_TYPE_2D;
        break;

        default:
        break;
    }

    spock::Image image = create_image(extent, type, format, usage, mipLevels);
    image.view = create_image_view(image, viewType, extent, 0, mipLevels, 0);

    //not the actual extent of the image, just the user input
    image.extent = extent;
    image.currentStage = VK_PIPELINE_STAGE_2_NONE;
    image.currentAccess = VK_ACCESS_2_NONE;

    return image;
}

VkCommandPool spock::create_command_pool(VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext                   = nullptr;
    commandPoolInfo.flags                   = flags;
    commandPoolInfo.queueFamilyIndex        = spock::ctx.graphicsQueueFamily;
    VkCommandPool pool;
    VK_CHECK(vkCreateCommandPool(spock::ctx.device, &commandPoolInfo, nullptr, &pool));
    return pool;
}

VkCommandBuffer spock::create_command_buffer(VkCommandPool pool, VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.pNext                       = nullptr;
    cmdAllocInfo.commandPool                 = pool;
    cmdAllocInfo.commandBufferCount          = 1;
    cmdAllocInfo.level                       = level;
    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(spock::ctx.device, &cmdAllocInfo, &cmd));
    return cmd;
}

VkFence spock::create_fence(VkFenceCreateFlagBits flags)
{
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;
    info.flags             = flags;
    VkFence fence;
    VK_CHECK(vkCreateFence(spock::ctx.device, &info, nullptr, &fence));

    return fence;
}

VkSemaphore spock::create_semaphore()
{
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext                 = nullptr;
    info.flags                 = 0;
    VkSemaphore semaphore;
    VK_CHECK(vkCreateSemaphore(spock::ctx.device, &info, nullptr, &semaphore));
    return semaphore;
}
void spock::destroy_image(Image image)
{
    vmaDestroyImage(spock::ctx.allocator, image.image, image.allocation);
    if (image.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ctx.device, image.view, nullptr);
    }
}

void spock::destroy_image_view(VkImageView view)
{
    vkDestroyImageView(ctx.device, view, nullptr);
}

void spock::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder{ctx.physicalDevice, ctx.device, ctx.surface};
    ctx.swapchain.format   = VK_FORMAT_B8G8R8A8_UNORM;
    auto swap_ret = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{.format = ctx.swapchain.format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      .set_desired_min_image_count(3)
                                      //use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                                      .set_desired_extent(width, height)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                                      .build();

    if (!swap_ret)
    {
        std::cerr << "Failed to create swapchain. Error: " << swap_ret.error().message() << '\n';
    }

    vkb::Swapchain vkbSwapchain = swap_ret.value();

    ctx.swapchain.extent = vkbSwapchain.extent;
    //store swapchain and its related images
    ctx.swapchain.swapchain = vkbSwapchain.swapchain;

    auto views = vkbSwapchain.get_image_views().value();
    auto images = vkbSwapchain.get_images().value();

    ctx.swapchain.images.resize(images.size());
    ctx.swapchain.views.resize(views.size());

    for (int i = 0; i < ctx.swapchain.images.size(); i++)
    {
        ctx.swapchain.images[i].image = images[i];
        ctx.swapchain.views[i] = views[i];
        ctx.swapchain.images[i].format = ctx.swapchain.format;
        ctx.swapchain.images[i].currentStage = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        ctx.swapchain.images[i].currentAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }
}

void spock::cleanup() {
    if (!ctx.initialised)
        return;

    vkDeviceWaitIdle(ctx.device);

    destroyQueue.flush();

    destroy_swapchain();

    vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    vkDestroyDevice(ctx.device, nullptr);

    vkb::destroy_debug_utils_messenger(ctx.instance, ctx.debugMessenger);
    vkDestroyInstance(ctx.instance, nullptr);
}

void spock::destroy_buffer(Buffer buffer) {
    vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);
}
