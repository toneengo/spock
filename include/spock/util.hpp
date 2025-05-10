#pragma once
#include <vulkan/vulkan_core.h>
#include <cassert>
#include <cstdio>
#include <initializer_list>

namespace spock {
    inline VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask) {
        VkImageSubresourceRange subImage{};
        subImage.aspectMask     = aspectMask;
        subImage.baseMipLevel   = 0;
        subImage.levelCount     = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount     = VK_REMAINING_ARRAY_LAYERS;
        return subImage;
    }

    struct BarrierMask {
        VkPipelineStageFlags2 srcStageMask;
        VkAccessFlags2        srcAccessMask;
        VkPipelineStageFlags2 dstStageMask;
        VkAccessFlags2        dstAccessMask;
    };

    inline void image_barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
                              VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                              VkPipelineStageFlags2 dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                              VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT) {
        VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask  = srcStageMask;
        imageBarrier.srcAccessMask = srcAccessMask;
        imageBarrier.dstStageMask  = dstStageMask;
        imageBarrier.dstAccessMask = dstAccessMask;

        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;

        VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = image_subresource_range(aspectMask);
        imageBarrier.image            = image;

        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;

        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }
    inline void buffer_barrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size,
                              VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                              VkPipelineStageFlags2 dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                              VkAccessFlags2        dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT) {
        VkBufferMemoryBarrier2 bufferBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .buffer = buffer,
            .offset = offset,
            .size = size
        };

        VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .imageMemoryBarrierCount = 0,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers = &bufferBarrier
        };

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    inline void clear_image(VkCommandBuffer cmd, VkImage image, VkImageLayout layout, VkClearColorValue color) {
        VkImageSubresourceRange subImage{};
        subImage.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        subImage.baseMipLevel   = 0;
        subImage.levelCount     = VK_REMAINING_MIP_LEVELS;
        subImage.baseArrayLayer = 0;
        subImage.layerCount     = VK_REMAINING_ARRAY_LAYERS;
 
        vkCmdClearColorImage(cmd, image, layout, &color, 1, &subImage);
    }

    inline void blit(VkCommandBuffer cmd, VkImage src, VkImage dst, VkRect2D srcRect, VkRect2D dstRect, VkFilter filter) {
        VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

        blitRegion.srcOffsets[0].x = srcRect.offset.x;
        blitRegion.srcOffsets[0].y = srcRect.offset.y;
        blitRegion.srcOffsets[0].z = 0;

        blitRegion.dstOffsets[0].x = dstRect.offset.x;
        blitRegion.dstOffsets[0].y = dstRect.offset.y;
        blitRegion.dstOffsets[0].z = 0;

        blitRegion.srcOffsets[1].x = srcRect.offset.x + srcRect.extent.width;
        blitRegion.srcOffsets[1].y = srcRect.offset.y + srcRect.extent.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstRect.offset.x + dstRect.extent.width;
        blitRegion.dstOffsets[1].y = dstRect.offset.y + dstRect.extent.height;
        blitRegion.dstOffsets[1].z = 1;

        /*
        auto& a = blitRegion.srcOffsets;
        auto& b = blitRegion.dstOffsets;
        printf("src region: %d, %d, %d, %d\n", a[0].x, a[0].y, a[1].x, a[1].y);
        printf("dst region: %d, %d, %d, %d\n\n", b[0].x, b[0].y, b[1].x, b[1].y);
        */

        blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount     = 1;
        blitRegion.srcSubresource.mipLevel       = 0;

        blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount     = 1;
        blitRegion.dstSubresource.mipLevel       = 0;

        VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
        blitInfo.dstImage       = dst;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage       = src;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter         = filter;
        blitInfo.regionCount    = 1;
        blitInfo.pRegions       = &blitRegion;

        vkCmdBlitImage2(cmd, &blitInfo);
    }
    inline void           set_viewport(const VkCommandBuffer& cmd, float x, float y, float width, float height, float minDepth = 0.f, float maxDepth = 1.f)
    {
        VkViewport viewport = {};
        viewport.x          = x;
        viewport.y          = y;
        viewport.width      = width;
        viewport.height     = height;
        viewport.minDepth   = minDepth;
        viewport.maxDepth   = maxDepth;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
    } 
    
    inline void set_scissor(const VkCommandBuffer& cmd, float x, float y, float width, float height)
    {
        VkRect2D scissor      = {};
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        scissor.extent.width  = width;
        scissor.extent.height = height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    struct RenderingAttachmentInfo {
        VkImageView imageView = VK_NULL_HANDLE;
        bool clear = false;
        VkClearValue clearValue = {};
    };

    /*
    Simple dynamic rendering start, image layouts default to COLOR_ATTACHMENT_OPTIMAL, etc.
    end_dynamic_rendering must follow.
     *replaces:
        VkRenderingAttachmentInfo colorAttachment = info::color_attachment(spock::ctx.swapchain.imageViews[swapchainImageIndex], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingInfo           renderInfo      = info::rendering(spock::ctx.swapchain.extent, &colorAttachment, nullptr);
        vkCmdBeginRendering(cmd, &renderInfo);
    */
    inline void begin_dynamic_rendering(VkCommandBuffer cmd, VkExtent2D extent, std::initializer_list<RenderingAttachmentInfo> color, RenderingAttachmentInfo depth = {}, RenderingAttachmentInfo stencil = {}, uint32_t layerCount = 1, VkRenderingFlags flags = 0)
    {
        //max 16 color attachments
        VkRenderingAttachmentInfo colorAttachments[16];
        VkRenderingAttachmentInfo depthAttachment = {};
        VkRenderingAttachmentInfo stencilAttachment = {};
        int i = 0;
        for (auto& c : color)
        {
            colorAttachments[i++] = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext       = nullptr,
                .imageView   = c.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp      = c.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = c.clearValue,
            };
        }

        if (depth.imageView != VK_NULL_HANDLE)
        {
            depthAttachment = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext       = nullptr,
                .imageView   = depth.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp      = depth.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = depth.clearValue,
            };
        }

        if (stencil.imageView != VK_NULL_HANDLE)
        {
            stencilAttachment = {
                .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext       = nullptr,
                .imageView   = stencil.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
                .loadOp      = stencil.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue  = stencil.clearValue,
            };
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pNext                = nullptr;
        renderingInfo.flags                = flags;
        renderingInfo.renderArea           = {0, 0, extent.width, extent.height};
        renderingInfo.layerCount           = layerCount;
        renderingInfo.viewMask             = 0;
        renderingInfo.colorAttachmentCount = i;
        renderingInfo.pColorAttachments    = i == 0 ? nullptr : colorAttachments;
        renderingInfo.pDepthAttachment     = depth.imageView == VK_NULL_HANDLE ? nullptr : &depthAttachment;
        renderingInfo.pStencilAttachment   = stencil.imageView == VK_NULL_HANDLE ? nullptr : &stencilAttachment;
        vkCmdBeginRendering(cmd, &renderingInfo);
    }

    inline void end_dynamic_rendering(VkCommandBuffer cmd)
    {
        vkCmdEndRendering(cmd);
    }
#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            printf("Detected Vulkan error: %d\n", err); \
            abort();                                                    \
        }                                                               \
    } while (0)

    inline void log_line(const char* source)
    {
        char buf[512];

        uint32_t linenum = 1;
        uint32_t bufpos = 0;

        for (const char* c = source; *c; c++)
        {
            assert(bufpos < 512);
            if (*c == '\n')
            {
                buf[bufpos] = '\0';
                printf("%5u | %s\n", linenum, buf);
                linenum++;
                bufpos = 0;
                continue;
            }
            
            buf[bufpos++] = *c;
        }
    }
}
