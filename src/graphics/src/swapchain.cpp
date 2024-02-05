#include "logger.hpp"
#include "logical_device.hpp"
#include "physical_device.hpp"
#include <algorithm>
#include <swapchain.hpp>

namespace Humongous
{
SwapChain::SwapChain(Window& window, PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice, VkSwapchainKHR* oldSwap)
    : m_logicalDevice(logicalDevice)
{
    CreateSwapChain(window, physicalDevice, oldSwap);
    CreateImageViews();
}

SwapChain::~SwapChain()
{
    for(auto imageView: m_imageViews) { vkDestroyImageView(m_logicalDevice.GetVkDevice(), imageView, nullptr); }

    vkDestroySwapchainKHR(m_logicalDevice.GetVkDevice(), m_swapChain, nullptr);
    HGINFO("Destroyed SwapChain");
}

void SwapChain::CreateSwapChain(Window& window, PhysicalDevice& physicalDevice, VkSwapchainKHR* oldSwap)
{
    HGINFO("Creating SwapChain...");
    PhysicalDevice::SwapChainSupportDetails details = physicalDevice.QuerySwapChainSupport(physicalDevice.GetVkPhysicalDevice());

    VkSurfaceFormat2KHR surfaceFormat = ChooseSurfaceFormat(details.formats);
    VkPresentModeKHR    presentMode = ChoosePresentMode(details.presentModes);
    VkExtent2D          extent = ChooseExtent(details.capabilities, window);

    u32 imageCount = details.capabilities.surfaceCapabilities.minImageCount + 1;
    if(details.capabilities.surfaceCapabilities.maxImageCount > 0 && imageCount > details.capabilities.surfaceCapabilities.maxImageCount)
    {
        imageCount = details.capabilities.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = physicalDevice.GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    PhysicalDevice::QueueFamilyData indices = physicalDevice.FindQueueFamilies(physicalDevice.GetVkPhysicalDevice());
    u32                             queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if(indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else { createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; }

    createInfo.preTransform = details.capabilities.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwap == nullptr ? VK_NULL_HANDLE : *oldSwap;

    if(vkCreateSwapchainKHR(m_logicalDevice.GetVkDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        HGERROR("Failed to create swapchain!");
    }
    else
    {
        if(oldSwap != nullptr) { vkDestroySwapchainKHR(m_logicalDevice.GetVkDevice(), *oldSwap, nullptr); }
        m_surfaceFormat = surfaceFormat.surfaceFormat.format;
        m_extent = extent;
    }

    HGINFO("Created SwapChain");

    vkGetSwapchainImagesKHR(m_logicalDevice.GetVkDevice(), m_swapChain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_logicalDevice.GetVkDevice(), m_swapChain, &imageCount, m_images.data());
    HGINFO("Got %d swapchain images", imageCount);
}

void SwapChain::CreateImageViews()
{
    m_imageViews.resize(m_images.size());

    for(size_t i = 0; i < m_imageViews.size(); i++)
    {
        VkImageViewCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        createInfo.image = m_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_surfaceFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(m_logicalDevice.GetVkDevice(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
        {
            HGERROR("Failed to create image view!");
        }
    }
}

void SwapChain::TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    imageBarrier.pNext = nullptr;

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.aspectMask = aspectMask;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void SwapChain::CopyImageToImage(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

VkSurfaceFormat2KHR SwapChain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& formats)
{
    VkSurfaceFormat2KHR comp{.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                             .pNext = nullptr,
                             .surfaceFormat = {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};

    for(const auto& format: formats)
    {
        if(format.surfaceFormat.format == comp.surfaceFormat.format && format.surfaceFormat.colorSpace == comp.surfaceFormat.colorSpace)
        {
            return format;
        }
    }

    // TODO: some sort of error handling
    return formats[0];
}

VkPresentModeKHR SwapChain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    for(const auto& mode: presentModes)
    {
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR) { return mode; }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseExtent(const VkSurfaceCapabilities2KHR& capabilities, Window& window)
{
    if(capabilities.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.surfaceCapabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window.GetWindow(), &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.surfaceCapabilities.minImageExtent.width,
                                        capabilities.surfaceCapabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.surfaceCapabilities.minImageExtent.height,
                                         capabilities.surfaceCapabilities.maxImageExtent.height);

        return actualExtent;
    }
}
} // namespace Humongous
