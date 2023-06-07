//
// Created by nudelerde on 05.06.23.
//

#pragma once
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace cr::vulkan {

struct PhysicalDevice;
struct LogicalDevice;
struct Window;
struct Instance;
struct SwapChain;
struct Pipeline;
struct Queue;

using CommandBuffer = vk::CommandBuffer;

struct Semaphore {
    Semaphore() = default;
    Semaphore(const Semaphore&) = delete;
    Semaphore(Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore& operator=(Semaphore&&) = delete;
    ~Semaphore();

    vk::Semaphore semaphore;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct Fence {
    Fence() = default;
    Fence(const Fence&) = delete;
    Fence(Fence&&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence& operator=(Fence&&) = delete;
    ~Fence();

    void wait();
    void reset();

    vk::Fence fence;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct CommandPool {
    CommandPool() = default;
    CommandPool(const CommandPool&) = delete;
    CommandPool(CommandPool&&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;
    ~CommandPool();

    CommandBuffer createCommandBuffer();

    vk::CommandPool commandPool;
    std::shared_ptr<Queue> queue;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct Framebuffers {
    Framebuffers() = default;
    Framebuffers(const Framebuffers&) = delete;
    Framebuffers(Framebuffers&&) = delete;
    Framebuffers& operator=(const Framebuffers&) = delete;
    Framebuffers& operator=(Framebuffers&&) = delete;
    ~Framebuffers();

    std::shared_ptr<LogicalDevice> logicalDevice;
    std::vector<vk::Framebuffer> framebuffers;
    std::shared_ptr<SwapChain> swapChain{};
    std::shared_ptr<Pipeline> pipeline{};
};

struct Pipeline {
    Pipeline() = default;
    Pipeline(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;
    ~Pipeline();

    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    vk::RenderPass renderPass;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct ShaderModule {
    ShaderModule() = default;
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule& operator=(ShaderModule&&) = delete;
    ~ShaderModule();

    vk::ShaderModule shaderModule;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct Surface {
    Surface() = default;
    Surface(const Surface&) = delete;
    Surface(Surface&&) = delete;
    Surface& operator=(const Surface&) = delete;
    Surface& operator=(Surface&&) = delete;
    ~Surface();

    std::shared_ptr<Instance> instance;
    std::shared_ptr<Window> window;
    vk::SurfaceKHR surface;
};

struct SwapChain {
    SwapChain() = default;
    SwapChain(const SwapChain&) = delete;
    SwapChain(SwapChain&&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;
    SwapChain& operator=(SwapChain&&) = delete;
    ~SwapChain();

    std::optional<uint32_t> acquireNextImage(const std::shared_ptr<Semaphore>& semaphore);
    bool present(const std::shared_ptr<Semaphore>& waitSemaphore, uint32_t imageIndex);

    vk::SwapchainKHR swapChain;
    std::shared_ptr<LogicalDevice> logicalDevice;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    vk::Extent2D extent;
    vk::Format format{};
    vk::PresentModeKHR presentMode{};
};

struct Queue : public std::enable_shared_from_this<Queue> {
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue& operator=(Queue&&) = delete;
    ~Queue() = default;

    void submit(CommandBuffer commandBuffer, const std::shared_ptr<Semaphore>& waitSemaphore,
                const std::shared_ptr<Semaphore>& signalSemaphore, const std::shared_ptr<Fence>& fence) const;

    vk::Queue queue;
    uint32_t familyIndex{};
    std::weak_ptr<LogicalDevice> logicalDevice;
};

struct SwapChainInfo {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    [[nodiscard]] vk::SurfaceFormatKHR chooseSurfaceFormat() const;
    [[nodiscard]] vk::PresentModeKHR choosePresentMode() const;
    [[nodiscard]] vk::Extent2D chooseExtent(const std::shared_ptr<Window>& window) const;
};

struct LogicalDevice : public std::enable_shared_from_this<LogicalDevice> {
    LogicalDevice() = default;
    LogicalDevice(const LogicalDevice&) = delete;
    LogicalDevice(LogicalDevice&&) = delete;
    LogicalDevice& operator=(const LogicalDevice&) = delete;
    LogicalDevice& operator=(LogicalDevice&&) = delete;
    ~LogicalDevice();

    vk::Device device;
    std::vector<std::shared_ptr<Queue>> queues;
    std::shared_ptr<Queue> graphicsQueue;
    std::shared_ptr<Queue> presentQueue;

    std::shared_ptr<SwapChain> createSwapChain(const std::shared_ptr<Surface>& surface, const SwapChainInfo& info, vk::Extent2D extent,
                                               vk::PresentModeKHR presentMode, vk::SurfaceFormatKHR surfaceFormat, uint32_t bufferCount);
    std::shared_ptr<ShaderModule> createShaderModule(const std::vector<char>& bytecode);
    std::shared_ptr<Pipeline> createPipeline(const std::shared_ptr<ShaderModule>& vertexShader,
                                             const std::shared_ptr<ShaderModule>& fragmentShader,
                                             vk::PrimitiveTopology topology,
                                             const std::shared_ptr<SwapChain>& swapChain);
    std::shared_ptr<Framebuffers> createFramebuffers(const std::shared_ptr<SwapChain>& swapChain,
                                                     const std::shared_ptr<Pipeline>& pipeline);
    std::shared_ptr<CommandPool> createCommandPool(const std::shared_ptr<Queue>& queue);
    std::shared_ptr<Fence> createFence(bool signaled = false);
    std::shared_ptr<Semaphore> createSemaphore();
    void waitIdle() const;
    void recreateSwapChain(std::shared_ptr<SwapChain>& swapChain, std::shared_ptr<Framebuffers>& framebuffers,
                           const std::shared_ptr<Surface>& surface, const SwapChainInfo& info, vk::Extent2D extent,
                           vk::PresentModeKHR presentMode, vk::SurfaceFormatKHR surfaceFormat, uint32_t bufferCount);

    std::shared_ptr<PhysicalDevice> physicalDevice;
};

struct PhysicalDevice : public std::enable_shared_from_this<PhysicalDevice> {
    PhysicalDevice() = default;
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(PhysicalDevice&&) = delete;
    ~PhysicalDevice() = default;

    void updateFamilyIndices();
    std::shared_ptr<LogicalDevice> createLogicalDevice();
    std::string getDeviceName() const;
    SwapChainInfo getSwapChainInfo() const;

    vk::PhysicalDevice physicalDevice;
    std::optional<uint32_t> graphicsFamily{};
    std::optional<uint32_t> presentFamily{};
    std::optional<uint32_t> computeFamily{};
    std::optional<uint32_t> transferFamily{};
    std::optional<uint32_t> sparseBindingFamily{};
    std::optional<uint32_t> protectedFamily{};
    std::weak_ptr<Surface> surface;
};

struct Instance;

std::shared_ptr<Instance> createInstance();

struct Instance : public std::enable_shared_from_this<Instance> {
    Instance() = default;
    Instance(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&) = delete;
    ~Instance();

    [[nodiscard]] static std::vector<vk::ExtensionProperties> getExtensions();

    [[nodiscard]] std::vector<vk::PhysicalDevice> getPhysicalDevices() const;
    [[nodiscard]] std::shared_ptr<PhysicalDevice> createPhysicalDevice(uint32_t index, const std::shared_ptr<Surface>& surface) const;
    [[nodiscard]] std::shared_ptr<Surface> createSurface(const std::shared_ptr<Window>& window);


    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
};

void prepareCommandBuffer(const CommandBuffer& commandBuffer, const std::shared_ptr<Framebuffers>& framebuffers,
                          const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<SwapChain>& swapChain, size_t index);

}// namespace cr::vulkan