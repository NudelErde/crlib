//
// Created by nudelerde on 05.06.23.
//

constexpr auto WIDTH = 800;
constexpr auto HEIGHT = 600;
constexpr auto MAX_FRAMES_IN_FLIGHT = 2;

#include "crvulkan/vulkan.h"
#include "crvulkan/window.h"
#include <fstream>

int main() {
    auto window = cr::vulkan::createWindow(WIDTH, HEIGHT);

    auto instance = cr::vulkan::createInstance();
    auto surface = instance->createSurface(window);

    auto physicalDevice = instance->createPhysicalDevice(0, surface);
    auto logicalDevice = physicalDevice->createLogicalDevice();
    auto swapChainInfo = physicalDevice->getSwapChainInfo();
    auto swapChain = logicalDevice->createSwapChain(surface, swapChainInfo,
                                                    swapChainInfo.chooseExtent(window),
                                                    swapChainInfo.choosePresentMode(),
                                                    swapChainInfo.chooseSurfaceFormat(),
                                                    swapChainInfo.capabilities.minImageCount + 1);
    std::ifstream vert_file("./triangle_vert.spv", std::ios::binary);
    std::ifstream frag_file("./triangle_frag.spv", std::ios::binary);
    std::vector<char> vert_code((std::istreambuf_iterator<char>(vert_file)), std::istreambuf_iterator<char>());
    std::vector<char> frag_code((std::istreambuf_iterator<char>(frag_file)), std::istreambuf_iterator<char>());
    auto vert_shader_module = logicalDevice->createShaderModule(vert_code);
    auto frag_shader_module = logicalDevice->createShaderModule(frag_code);
    auto pipeline = logicalDevice->createPipeline(vert_shader_module, frag_shader_module, vk::PrimitiveTopology::eTriangleList, swapChain);
    auto framebuffers = logicalDevice->createFramebuffers(swapChain, pipeline);
    auto commandPool = logicalDevice->createCommandPool(logicalDevice->graphicsQueue);

    std::vector<std::shared_ptr<cr::vulkan::Fence>> inFlightFences;
    std::vector<std::shared_ptr<cr::vulkan::Semaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<cr::vulkan::Semaphore>> renderFinishedSemaphores;
    std::vector<cr::vulkan::CommandBuffer> commandBuffers;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        inFlightFences.push_back(logicalDevice->createFence(true));
        imageAvailableSemaphores.push_back(logicalDevice->createSemaphore());
        renderFinishedSemaphores.push_back(logicalDevice->createSemaphore());
    }
    for (int i = 0; i < swapChain->images.size(); ++i) {
        commandBuffers.push_back(commandPool->createCommandBuffer());
        cr::vulkan::prepareCommandBuffer(commandBuffers[i], framebuffers, pipeline, swapChain, i);
    }

    int currentFrame = 0;

    auto lastSize = window->getFramebufferSize();
    while (!window->shouldClose()) {
        window->pollEvents();
        if (window->getFramebufferSize() == cr::math::cvector<uint32_t, 2>(0, 0))
            continue;
        bool resized = false;
        if (window->getFramebufferSize() != lastSize) {
            resized = true;
            lastSize = window->getFramebufferSize();
        }

        int index = currentFrame % MAX_FRAMES_IN_FLIGHT;

        inFlightFences[index]->wait();
        auto res = swapChain->acquireNextImage(imageAvailableSemaphores[index]);
        if (!res) {
            swapChainInfo = physicalDevice->getSwapChainInfo();
            logicalDevice->recreateSwapChain(swapChain, framebuffers, surface, swapChainInfo,
                                             swapChainInfo.chooseExtent(window),
                                             swapChainInfo.choosePresentMode(),
                                             swapChainInfo.chooseSurfaceFormat(),
                                             swapChainInfo.capabilities.minImageCount + 1);
            for (int i = 0; i < swapChain->images.size(); ++i) {
                commandBuffers[i].reset();
                cr::vulkan::prepareCommandBuffer(commandBuffers[i], framebuffers, pipeline, swapChain, i);
            }
            continue;
        }
        inFlightFences[index]->reset();
        uint32_t image = *res;
        logicalDevice->graphicsQueue->submit(commandBuffers[image], imageAvailableSemaphores[index], renderFinishedSemaphores[index], inFlightFences[index]);
        if (!swapChain->present(renderFinishedSemaphores[index], image) || resized) {
            swapChainInfo = physicalDevice->getSwapChainInfo();
            logicalDevice->recreateSwapChain(swapChain, framebuffers, surface, swapChainInfo,
                                             swapChainInfo.chooseExtent(window),
                                             swapChainInfo.choosePresentMode(),
                                             swapChainInfo.chooseSurfaceFormat(),
                                             swapChainInfo.capabilities.minImageCount + 1);
            for (int i = 0; i < swapChain->images.size(); ++i) {
                commandBuffers[i].reset();
                cr::vulkan::prepareCommandBuffer(commandBuffers[i], framebuffers, pipeline, swapChain, i);
            }
        }
        currentFrame++;
    }
    logicalDevice->waitIdle();
}
