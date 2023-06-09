//
// Created by nudelerde on 05.06.23.
//

#include "vulkan.h"
#include "GLFW/glfw3.h"
#include "crutil/comptime.h"
#include "window.h"
#include <iostream>
#include <limits>
#include <map>
#include <set>

static constexpr std::array<const char*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

// debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

static std::map<std::string, void (*)()> functionPointers;

template<typename Res = void, typename... Args>
Res proxyCall(const cr::vulkan::Instance& instance, const char* funcName, Args... args) {
    if (functionPointers.find(funcName) == functionPointers.end()) {
        functionPointers[funcName] = vkGetInstanceProcAddr(instance.instance, funcName);
    }
    auto func = reinterpret_cast<Res (*)(Args...)>(functionPointers[funcName]);
    if (func == nullptr) {
        throw std::runtime_error("Failed to load function pointer for " + std::string(funcName));
    }
    return func(args...);
}

namespace cr::vulkan {

static bool hasValidationSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const auto& layer : validationLayers) {
        bool found = false;
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(layer, availableLayer.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

static vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo() {
    return {
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback};
}

static std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if constexpr (cr::compiletime::debug) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

Instance::~Instance() {
    if (debugMessenger)
        proxyCall(*this, "vkDestroyDebugUtilsMessengerEXT", instance, debugMessenger, nullptr);
    if (instance)
        instance.destroy();
}

std::vector<vk::ExtensionProperties> Instance::getExtensions() {
    return vk::enumerateInstanceExtensionProperties();
}

std::vector<vk::PhysicalDevice> Instance::getPhysicalDevices() const {
    return instance.enumeratePhysicalDevices();
}

std::shared_ptr<PhysicalDevice>
Instance::createPhysicalDevice(uint32_t index, const std::shared_ptr<Surface>& surface) const {
    auto physicalDevice = std::make_shared<PhysicalDevice>();
    physicalDevice->physicalDevice = getPhysicalDevices()[index];
    physicalDevice->surface = surface;
    physicalDevice->updateFamilyIndices();
    return physicalDevice;
}

std::shared_ptr<Instance> createInstance() {
    auto instance = std::make_unique<Instance>();

    auto appInfo = vk::ApplicationInfo("Vulkan", 1, "No Engine", 1, VK_API_VERSION_1_0);
    auto createInfo = vk::InstanceCreateInfo()
                              .setPApplicationInfo(&appInfo);

    auto extensions = getRequiredExtensions();
    createInfo.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
            .setPpEnabledExtensionNames(extensions.data());

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if constexpr (cr::compiletime::debug) {
        if (hasValidationSupport()) {
            createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()))
                    .setPpEnabledLayerNames(validationLayers.data());
        } else {
            throw std::runtime_error("validation layers required, but not available!");
        }
        debugCreateInfo = getDebugMessengerCreateInfo();
        createInfo.setPNext(&debugCreateInfo);
    }

    auto res = vk::createInstance(&createInfo, nullptr, &instance->instance);
    if (res != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create instance!");
    }

    if constexpr (cr::compiletime::debug) {
        proxyCall(*instance, "vkCreateDebugUtilsMessengerEXT", instance->instance, &debugCreateInfo, nullptr,
                  &instance->debugMessenger);
    }

    return instance;
}

void PhysicalDevice::updateFamilyIndices() {
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    auto surface_ptr = surface.lock();
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily = i;
        }
        if (surface_ptr && physicalDevice.getSurfaceSupportKHR(i, surface_ptr->surface)) {
            presentFamily = i;
        }
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) {
            computeFamily = i;
        }
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) {
            transferFamily = i;
        }
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eSparseBinding) {
            sparseBindingFamily = i;
        }
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eProtected) {
            protectedFamily = i;
        }
    }
}

std::shared_ptr<LogicalDevice> PhysicalDevice::createLogicalDevice() {
    auto logicalDevice = std::make_shared<LogicalDevice>();
    logicalDevice->physicalDevice = shared_from_this();

    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float queuePriority = 1.0f;
    std::set<uint32_t> requestedQueueFamilies = {graphicsFamily.value(), presentFamily.value()};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    for (auto family : requestedQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setQueueFamilyIndex(family)
                .setQueueCount(1)
                .setPQueuePriorities(&queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::DeviceCreateInfo createInfo;
    createInfo.setPQueueCreateInfos(queueCreateInfos.data())
            .setQueueCreateInfoCount(queueCreateInfos.size())
            .setPEnabledFeatures(&deviceFeatures)
            .setEnabledExtensionCount(deviceExtensions.size())
            .setPpEnabledExtensionNames(deviceExtensions.data());
    if constexpr (cr::compiletime::debug) {
        createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()))
                .setPpEnabledLayerNames(validationLayers.data());
    }

    auto res = physicalDevice.createDevice(&createInfo, nullptr, &logicalDevice->device);
    if (res != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create logical device!");
    }

    logicalDevice->graphicsQueue = std::make_shared<Queue>();
    logicalDevice->graphicsQueue->queue = logicalDevice->device.getQueue(graphicsFamily.value(), 0);
    logicalDevice->graphicsQueue->familyIndex = graphicsFamily.value();
    logicalDevice->graphicsQueue->logicalDevice = logicalDevice;
    logicalDevice->presentQueue = std::make_shared<Queue>();
    logicalDevice->presentQueue->queue = logicalDevice->device.getQueue(presentFamily.value(), 0);
    logicalDevice->presentQueue->familyIndex = presentFamily.value();
    logicalDevice->presentQueue->logicalDevice = logicalDevice;
    logicalDevice->queues.push_back(logicalDevice->graphicsQueue);
    logicalDevice->queues.push_back(logicalDevice->presentQueue);

    return logicalDevice;
}

std::string PhysicalDevice::getDeviceName() const {
    return physicalDevice.getProperties().deviceName;
}

LogicalDevice::~LogicalDevice() {
    if (device)
        device.destroy();
}

std::shared_ptr<Surface> Instance::createSurface(const std::shared_ptr<Window>& window) {
    auto surface = std::make_shared<Surface>();
    surface->instance = shared_from_this();
    surface->window = window;
    VkSurfaceKHR surfaceHandle;
    auto res = glfwCreateWindowSurface(instance, window->window, nullptr, &surfaceHandle);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface->surface = surfaceHandle;
    return surface;
}

Surface::~Surface() {
    if (surface)
        instance->instance.destroySurfaceKHR(surface);
}

ShaderModule::~ShaderModule() {
    if (shaderModule)
        logicalDevice->device.destroyShaderModule(shaderModule);
}

std::shared_ptr<ShaderModule> LogicalDevice::createShaderModule(const std::vector<char>& bytecode) {
    auto shaderModule = std::make_shared<ShaderModule>();
    shaderModule->logicalDevice = shared_from_this();
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
    auto res = device.createShaderModule(&createInfo, nullptr, &shaderModule->shaderModule);
    if (res != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

std::shared_ptr<Framebuffers> LogicalDevice::createFramebuffers(const std::shared_ptr<SwapChain>& swapChain,
                                                                const std::shared_ptr<Pipeline>& pipeline) {
    auto framebuffers = std::make_shared<Framebuffers>();
    framebuffers->swapChain = swapChain;
    framebuffers->pipeline = pipeline;
    framebuffers->logicalDevice = shared_from_this();
    framebuffers->framebuffers.resize(swapChain->imageViews.size());
    framebuffers->generateFramebuffers();
    return framebuffers;
}

void Framebuffers::generateFramebuffers() {
    if (generated) {
        throw std::runtime_error("Framebuffers already generated");
    }
    for (size_t i = 0; i < swapChain->imageViews.size(); i++) {
        vk::ImageView attachments[] = {
                swapChain->imageViews[i]};

        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.renderPass = pipeline->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChain->extent.width;
        framebufferInfo.height = swapChain->extent.height;
        framebufferInfo.layers = 1;

        auto framebuffer = logicalDevice->device.createFramebuffer(framebufferInfo);
        framebuffers[i] = framebuffer;
    }
    generated = true;
}

void Framebuffers::regenerateFramebuffer(size_t i) {
    if (!generated) {
        throw std::runtime_error("Framebuffers not generated");
    }
    logicalDevice->device.destroyFramebuffer(framebuffers[i]);
    vk::ImageView attachments[] = {
            swapChain->imageViews[i]};

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = pipeline->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChain->extent.width;
    framebufferInfo.height = swapChain->extent.height;
    framebufferInfo.layers = 1;

    auto framebuffer = logicalDevice->device.createFramebuffer(framebufferInfo);
    framebuffers[i] = framebuffer;
}

void Framebuffers::deleteFramebuffers() {
    if (!generated) {
        throw std::runtime_error("Framebuffers not generated");
    }
    for (auto framebuffer : framebuffers) {
        logicalDevice->device.destroyFramebuffer(framebuffer);
    }
    generated = false;
}

Framebuffers::~Framebuffers() {
    for (auto framebuffer : framebuffers) {
        logicalDevice->device.destroyFramebuffer(framebuffer);
    }
}

std::shared_ptr<CommandPool> LogicalDevice::createCommandPool(const std::shared_ptr<Queue>& queue) {
    auto commandPool = std::make_shared<CommandPool>();
    commandPool->logicalDevice = shared_from_this();
    commandPool->queue = queue;
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.queueFamilyIndex = queue->familyIndex;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    auto pool = device.createCommandPool(poolInfo);
    commandPool->commandPool = pool;
    return commandPool;
}

CommandPool::~CommandPool() {
    logicalDevice->device.destroyCommandPool(commandPool);
}

CommandBuffer CommandPool::createCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    auto res = logicalDevice->device.allocateCommandBuffers(allocInfo);

    return res[0];
}

void CommandPool::freeCommandBuffer(CommandBuffer& commandBuffer) {
    logicalDevice->device.freeCommandBuffers(commandPool, commandBuffer);
}

void prepareCommandBuffer(const CommandBuffer& commandBuffer, const std::shared_ptr<Pipeline>& pipeline,
                          const std::shared_ptr<SwapChain>& swapChain, vk::Framebuffer& framebuffer,
                          size_t count, const std::span<std::shared_ptr<Buffer>>& vertexBuffers,
                          const std::shared_ptr<Buffer>& indexBuffer, vk::DescriptorSet descriptorSet) {
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits(0);
    beginInfo.pInheritanceInfo = nullptr;

    commandBuffer.begin(beginInfo);
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = pipeline->renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapChain->extent;
    vk::ClearValue clearColor;
    clearColor.color = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChain->extent.width;
    viewport.height = (float) swapChain->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, viewport);
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = swapChain->extent;
    commandBuffer.setScissor(0, scissor);

    if (descriptorSet) {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->pipelineLayout, 0, descriptorSet, nullptr);
    }

    if (!vertexBuffers.empty()) {
        std::vector<vk::DeviceSize> offsets(vertexBuffers.size(), 0);
        std::vector<vk::Buffer> buffers(vertexBuffers.size());
        for (size_t i = 0; i < vertexBuffers.size(); i++) {
            buffers[i] = vertexBuffers[i]->buffer;
        }
        commandBuffer.bindVertexBuffers(0, buffers, offsets);
    }
    if (indexBuffer) {
        commandBuffer.bindIndexBuffer(indexBuffer->buffer, 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(count, 1, 0, 0, 0);
    } else {
        commandBuffer.draw(count, 1, 0, 0);
    }
    commandBuffer.endRenderPass();
    commandBuffer.end();
}

Semaphore::~Semaphore() {
    logicalDevice->device.destroySemaphore(semaphore);
}

Fence::~Fence() {
    logicalDevice->device.destroyFence(fence);
}

std::shared_ptr<Fence> LogicalDevice::createFence(bool signaled) {
    auto fence = std::make_shared<Fence>();
    fence->logicalDevice = shared_from_this();
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits(0);
    auto f = device.createFence(fenceInfo);
    fence->fence = f;
    return fence;
}

std::shared_ptr<Semaphore> LogicalDevice::createSemaphore() {
    auto semaphore = std::make_shared<Semaphore>();
    semaphore->logicalDevice = shared_from_this();
    vk::SemaphoreCreateInfo semaphoreInfo;
    auto s = device.createSemaphore(semaphoreInfo);
    semaphore->semaphore = s;
    return semaphore;
}

void Fence::wait() {
    auto res = logicalDevice->device.waitForFences(fence, true, UINT64_MAX);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to wait for fence");
}

void Fence::reset() {
    logicalDevice->device.resetFences(fence);
}


void LogicalDevice::waitIdle() const {
    device.waitIdle();
}

uint32_t LogicalDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto memProperties = physicalDevice->physicalDevice.getMemoryProperties();
    uint32_t memoryTypeIndex = ~0;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }
    if (memoryTypeIndex == ~0) throw std::runtime_error("failed to find suitable memory type");
    return memoryTypeIndex;
}


vk::DescriptorSetLayoutBinding createSamplerLayoutBinding(uint32_t binding) {
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding = binding;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    return samplerLayoutBinding;
}

}// namespace cr::vulkan