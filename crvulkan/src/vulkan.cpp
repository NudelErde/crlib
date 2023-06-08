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


std::shared_ptr<SwapChain>
LogicalDevice::createSwapChain(const std::shared_ptr<Surface>& surface, const SwapChainInfo& info,
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

std::shared_ptr<Pipeline> LogicalDevice::createPipeline(const std::shared_ptr<ShaderModule>& vertexShader,
                                                        const std::shared_ptr<ShaderModule>& fragmentShader,
                                                        vk::PrimitiveTopology topology,
                                                        const std::shared_ptr<SwapChain>& swapChain,
                                                        std::span<const VertexInputAttributeDescription> attributeDescription,
                                                        std::span<const VertexInputBindingDescription> bindingDescription,
                                                        std::span<const UniformDescription> uniformDescriptions) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->logicalDevice = shared_from_this();

    vk::PipelineShaderStageCreateInfo vertexShaderStageInfo;
    vertexShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertexShaderStageInfo.module = vertexShader->shaderModule;
    vertexShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo;
    fragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentShaderStageInfo.module = fragmentShader->shaderModule;
    fragmentShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
    vertexInputInfo.vertexBindingDescriptionCount = bindingDescription.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    if (!uniformDescriptions.empty()) {
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.pBindings = uniformDescriptions.data();
        descriptorSetLayoutCreateInfo.bindingCount = uniformDescriptions.size();
        pipeline->descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &pipeline->descriptorSetLayout;
    } else {
        pipelineLayoutInfo.setLayoutCount = 0;
    }

    pipelineLayoutInfo.pushConstantRangeCount = 0;

    pipeline->pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapChain->format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlags();
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    pipeline->renderPass = device.createRenderPass(renderPassInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.layout = pipeline->pipelineLayout;
    pipelineInfo.renderPass = pipeline->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;
    auto pipe = device.createGraphicsPipeline(nullptr, pipelineInfo);
    if (pipe.result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    pipeline->pipeline = pipe.value;
    return pipeline;
}

Pipeline::~Pipeline() {
    if (pipelineLayout) {
        logicalDevice->device.destroyPipeline(pipeline);
        logicalDevice->device.destroyDescriptorSetLayout(descriptorSetLayout);
        logicalDevice->device.destroyPipelineLayout(pipelineLayout);
        logicalDevice->device.destroyRenderPass(renderPass);
    }
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

void Queue::submit(CommandBuffer commandBuffer, const std::shared_ptr<Semaphore>& waitSemaphore,
                   const std::shared_ptr<Semaphore>& signalSemaphore, const std::shared_ptr<Fence>& fence) const {
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;
    submitInfo.pWaitSemaphores = waitSemaphore ? &waitSemaphore->semaphore : nullptr;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = signalSemaphore ? 1 : 0;
    submitInfo.pSignalSemaphores = signalSemaphore ? &signalSemaphore->semaphore : nullptr;
    queue.submit(submitInfo, fence ? fence->fence : nullptr);
}

void Queue::transferBuffer(const std::shared_ptr<Buffer>& from, const std::shared_ptr<Buffer>& to,
                           CommandBuffer& buffer, const std::shared_ptr<Fence>& fence) const {
    buffer.reset();
    buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::BufferCopy copyRegion;
    copyRegion.size = from->size;

    buffer.copyBuffer(from->buffer, to->buffer, copyRegion);
    buffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;
    queue.submit(submitInfo, fence ? fence->fence : nullptr);
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

void LogicalDevice::waitIdle() const {
    device.waitIdle();
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

Buffer::~Buffer() {
    logicalDevice->device.destroyBuffer(buffer);
    logicalDevice->device.freeMemory(memory);
}

void Buffer::copyToBuffer(const std::span<const std::byte>& data) {
    void* ptr;
    auto res = logicalDevice->device.mapMemory(memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(), &ptr);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to map memory");
    memcpy(ptr, data.data(), data.size());
    logicalDevice->device.flushMappedMemoryRanges(vk::MappedMemoryRange(memory, 0, VK_WHOLE_SIZE));
    logicalDevice->device.unmapMemory(memory);
}

std::shared_ptr<Buffer> LogicalDevice::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
    auto buffer = std::make_shared<Buffer>();
    buffer->logicalDevice = shared_from_this();
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    auto b = device.createBuffer(bufferInfo);
    buffer->buffer = b;
    buffer->size = size;

    auto memRequirements = device.getBufferMemoryRequirements(buffer->buffer);
    auto memProperties = physicalDevice->physicalDevice.getMemoryProperties();
    uint32_t memoryTypeIndex = ~0;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }
    if (memoryTypeIndex == ~0) throw std::runtime_error("failed to find suitable memory type");
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    auto m = device.allocateMemory(allocInfo);
    buffer->memory = m;
    device.bindBufferMemory(buffer->buffer, buffer->memory, 0);
    return buffer;
}

Buffer::MemoryMapping Buffer::mapMemory() {
    Buffer::MemoryMapping mapping;
    mapping.buffer = shared_from_this();
    auto res = mapping.buffer->logicalDevice->device.mapMemory(memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(),
                                                               &mapping.data);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to map memory");
    return mapping;
}

Buffer::MemoryMapping::~MemoryMapping() {
    if (buffer)
        buffer->logicalDevice->device.unmapMemory(buffer->memory);
}

Buffer::MemoryMapping::MemoryMapping(Buffer::MemoryMapping&& other) noexcept {
    buffer = other.buffer;
    data = other.data;
    other.buffer = nullptr;
    other.data = nullptr;
}
Buffer::MemoryMapping& Buffer::MemoryMapping::operator=(Buffer::MemoryMapping&& other) noexcept {
    if (this == &other) return *this;
    if (data) {
        buffer->logicalDevice->device.unmapMemory(buffer->memory);
    }
    buffer = other.buffer;
    data = other.data;
    other.buffer = nullptr;
    other.data = nullptr;
    return *this;
}

std::shared_ptr<UniformPool> LogicalDevice::createUniformPool(uint32_t poolSize, const std::shared_ptr<Pipeline>& pipeline) {
    auto pool = std::make_shared<UniformPool>();
    vk::DescriptorPoolSize poolSizeInfo;
    poolSizeInfo.type = vk::DescriptorType::eUniformBuffer;
    poolSizeInfo.descriptorCount = poolSize;
    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSizeInfo;
    poolInfo.maxSets = poolSize;
    pool->pool = device.createDescriptorPool(poolInfo);
    pool->logicalDevice = shared_from_this();
    pool->sets.resize(poolSize);
    std::vector<vk::DescriptorSetLayout> layouts(poolSize, pipeline->descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = pool->pool;
    allocInfo.descriptorSetCount = poolSize;
    allocInfo.pSetLayouts = layouts.data();
    auto res = device.allocateDescriptorSets(&allocInfo, pool->sets.data());
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to allocate descriptor sets");
    return pool;
}

UniformPool::~UniformPool() {
    logicalDevice->device.destroyDescriptorPool(pool);
}

void UniformPool::bindBuffer(const std::shared_ptr<Buffer>& buffer, size_t setIndex, size_t buffer_offset, size_t buffer_size, size_t binding, size_t arrayElement, size_t count) {
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = buffer_offset;
    bufferInfo.range = buffer_size;
    vk::WriteDescriptorSet descriptorWrite;
    descriptorWrite.dstSet = sets[setIndex];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = count;
    descriptorWrite.pBufferInfo = &bufferInfo;
    logicalDevice->device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

void UniformPool::generateBuffers(size_t uniformSize) {
    uniformBuffers.clear();
    uniformMappings.clear();
    for (size_t i = 0; i < sets.size(); ++i) {
        auto buffer = logicalDevice->createBuffer(uniformSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                                          vk::MemoryPropertyFlagBits::eHostCoherent);
        uniformBuffers.push_back(buffer);
        uniformMappings.push_back(buffer->mapMemory());
        bindBuffer(buffer, i, 0, uniformSize, 0, 0, 1);
    }
}

}// namespace cr::vulkan