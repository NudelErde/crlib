//
// Created by nudelerde on 09.06.23.
//

#include "GLFW/glfw3.h"
#include "vulkan.h"
#include "window.h"
#include <limits>

namespace cr::vulkan {
std::optional<uint32_t> SwapChain::acquireNextImage(const std::shared_ptr<Semaphore>& semaphore) {
    try {
        auto res = logicalDevice->device.acquireNextImageKHR(swapChain, UINT64_MAX, semaphore->semaphore, nullptr);
        if (res.result == vk::Result::eSuccess) return res.value;
        if (res.result == vk::Result::eSuboptimalKHR) return res.value;
        if (res.result == vk::Result::eErrorOutOfDateKHR) return std::nullopt;
    } catch (
            vk::OutOfDateKHRError& e) {// this try catch is stupid bullshit, bruh ur already using return values >:(
        return std::nullopt;
    }
    throw std::runtime_error("failed to acquire next image");
}

bool SwapChain::present(const std::shared_ptr<Semaphore>& waitSemaphore, uint32_t imageIndex) {
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore->semaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    try {
        auto res = logicalDevice->presentQueue->queue.presentKHR(presentInfo);
        if (res == vk::Result::eSuboptimalKHR) return false;
        if (res == vk::Result::eErrorOutOfDateKHR) return false;
        if (res == vk::Result::eSuccess) return true;
    } catch (
            vk::OutOfDateKHRError& e) {// this try catch is stupid bullshit, bruh ur already using return values >:(
        return false;
    }
    throw std::runtime_error("failed to present swap chain image");
}

SwapChainInfo PhysicalDevice::getSwapChainInfo() const {
    SwapChainInfo info;

    auto surface_ptr = surface.lock();
    if (!surface_ptr) {
        throw std::runtime_error("surface not set");
    }

    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface_ptr->surface);
    info.capabilities = surfaceCapabilities;
    auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface_ptr->surface);
    info.formats = surfaceFormats;
    auto surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface_ptr->surface);
    info.presentModes = surfacePresentModes;

    return info;
}

void LogicalDevice::recreateSwapChain(std::shared_ptr<SwapChain>& swapChain, std::shared_ptr<Framebuffers>& framebuffers,
                                      const std::shared_ptr<Surface>& surface, const SwapChainInfo& info,
                                      vk::Extent2D extent,
                                      vk::PresentModeKHR presentMode, vk::SurfaceFormatKHR surfaceFormat,
                                      uint32_t bufferCount) {
    waitIdle();
    // TODO: this is hyper hacky, fix it using swapchain recreation (the good way with oldSwapChain ptr and destruction of in flight resources and stuff)
    auto pipeline = framebuffers->pipeline;
    framebuffers->deleteFramebuffers();
    for (auto& imageView : swapChain->imageViews) {
        device.destroyImageView(imageView);
    }
    device.destroySwapchainKHR(swapChain->swapChain);
    auto newSwapChain = createSwapChain(surface, info, extent, presentMode, surfaceFormat, bufferCount);
    swapChain->swapChain = newSwapChain->swapChain;
    swapChain->images = newSwapChain->images;
    swapChain->imageViews = newSwapChain->imageViews;
    swapChain->extent = newSwapChain->extent;
    swapChain->format = newSwapChain->format;
    newSwapChain->swapChain = nullptr;
    framebuffers->generateFramebuffers();
}

vk::SurfaceFormatKHR SwapChainInfo::chooseSurfaceFormat() const {
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return formats[0];
}

vk::PresentModeKHR SwapChainInfo::choosePresentMode() const {
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChainInfo::chooseExtent(const std::shared_ptr<Window>& window) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window->window, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}


std::shared_ptr<SwapChain> LogicalDevice::createSwapChain(const std::shared_ptr<Surface>& surface, const SwapChainInfo& info,
                                                          vk::Extent2D extent, vk::PresentModeKHR presentMode,
                                                          vk::SurfaceFormatKHR surfaceFormat, uint32_t bufferCount) {
    auto swapChain = std::make_shared<SwapChain>();
    swapChain->logicalDevice = shared_from_this();

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface = surface->surface;
    createInfo.minImageCount = bufferCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    if (presentQueue->familyIndex != graphicsQueue->familyIndex) {
        uint32_t queueFamilyIndices[] = {graphicsQueue->familyIndex, presentQueue->familyIndex};
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }
    createInfo.preTransform = info.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = nullptr;
    auto res = device.createSwapchainKHR(&createInfo, nullptr, &swapChain->swapChain);
    if (res != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create swap chain!");
    }

    swapChain->images = device.getSwapchainImagesKHR(swapChain->swapChain);
    swapChain->format = surfaceFormat.format;
    swapChain->extent = extent;
    swapChain->presentMode = presentMode;

    for (auto& image : swapChain->images) {
        vk::ImageViewCreateInfo ivCreateInfo;
        ivCreateInfo.image = image;
        ivCreateInfo.viewType = vk::ImageViewType::e2D;
        ivCreateInfo.format = swapChain->format;
        ivCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        ivCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        ivCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        ivCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        ivCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        ivCreateInfo.subresourceRange.baseMipLevel = 0;
        ivCreateInfo.subresourceRange.levelCount = 1;
        ivCreateInfo.subresourceRange.baseArrayLayer = 0;
        ivCreateInfo.subresourceRange.layerCount = 1;
        swapChain->imageViews.push_back(device.createImageView(ivCreateInfo));
    }

    return swapChain;
}

SwapChain::~SwapChain() {
    if (swapChain) {
        for (auto& imageView : imageViews) {
            logicalDevice->device.destroyImageView(imageView);
        }
        logicalDevice->device.destroySwapchainKHR(swapChain);
    }
}
}// namespace cr::vulkan