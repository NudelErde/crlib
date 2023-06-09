//
// Created by nudelerde on 05.06.23.
//

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vulkan/vulkan.hpp>

namespace cr::vulkan {

struct PhysicalDevice;
struct LogicalDevice;
struct Window;
struct Instance;
struct SwapChain;
struct Pipeline;
struct Queue;
struct CommandPool;
struct Buffer;
struct Fence;
struct Image;

using CommandBuffer = vk::CommandBuffer;
using VertexInputBindingDescription = vk::VertexInputBindingDescription;
using VertexInputAttributeDescription = vk::VertexInputAttributeDescription;
using UniformDescription = vk::DescriptorSetLayoutBinding;
namespace VertexAttributeFormat {
constexpr auto eFloatVec1 = vk::Format::eR32Sfloat;
constexpr auto eFloatVec2 = vk::Format::eR32G32Sfloat;
constexpr auto eFloatVec3 = vk::Format::eR32G32B32Sfloat;
constexpr auto eFloatVec4 = vk::Format::eR32G32B32A32Sfloat;
constexpr auto eSIntVec1 = vk::Format::eR32Sint;
constexpr auto eSIntVec2 = vk::Format::eR32G32Sint;
constexpr auto eSIntVec3 = vk::Format::eR32G32B32Sint;
constexpr auto eSIntVec4 = vk::Format::eR32G32B32A32Sint;
constexpr auto eUIntVec1 = vk::Format::eR32Uint;
constexpr auto eUIntVec2 = vk::Format::eR32G32Uint;
constexpr auto eUIntVec3 = vk::Format::eR32G32B32Uint;
constexpr auto eUIntVec4 = vk::Format::eR32G32B32A32Uint;
}// namespace VertexAttributeFormat

struct StagingBufferUpload {
    std::shared_ptr<Buffer> stagingBuffer{};
    std::shared_ptr<CommandPool> commandPool{};
    std::shared_ptr<Fence> fence{};
    CommandBuffer commandBuffer{};

    StagingBufferUpload() = default;
    StagingBufferUpload(const StagingBufferUpload&) = delete;
    StagingBufferUpload(StagingBufferUpload&&) = delete;
    StagingBufferUpload& operator=(const StagingBufferUpload&) = delete;
    StagingBufferUpload& operator=(StagingBufferUpload&&) = delete;
    ~StagingBufferUpload();
    void wait();

    static void waitAll(const std::span<std::shared_ptr<StagingBufferUpload>>& uploads);
};

struct Sampler {
    Sampler() = default;
    Sampler(const Sampler&) = delete;
    Sampler(Sampler&&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler& operator=(Sampler&&) = delete;
    ~Sampler();

    vk::Sampler sampler;
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct Image {
    Image() = default;
    Image(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;
    ~Image();
    vk::Image image;
    vk::ImageView imageView;
    vk::DeviceMemory memory;

    [[nodiscard]] std::shared_ptr<StagingBufferUpload> upload(const std::span<const std::byte>& data, const std::shared_ptr<CommandPool>& commandPool);
    void transitionLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout, CommandBuffer& commandBuffer) const;

    size_t width{};
    size_t height{};
    vk::Format format{};
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

struct Buffer : public std::enable_shared_from_this<Buffer> {
    Buffer() = default;

    Buffer(const Buffer&) = delete;

    Buffer(Buffer&&) = delete;

    Buffer& operator=(const Buffer&) = delete;

    Buffer& operator=(Buffer&&) = delete;

    ~Buffer();

    void copyToBuffer(const std::span<const std::byte>& data);

    [[nodiscard]] std::shared_ptr<StagingBufferUpload> copyToBufferUsingStagingBuffer(const std::span<const std::byte>& data, const std::shared_ptr<CommandPool>& commandPool);

    struct MemoryMapping {
        std::shared_ptr<Buffer> buffer{};
        void* data{};

        template<typename T>
        T* as() {
            return reinterpret_cast<T*>(data);
        }

        MemoryMapping(const MemoryMapping&) = delete;

        MemoryMapping(MemoryMapping&& other) noexcept;

        MemoryMapping& operator=(const MemoryMapping&) = delete;

        MemoryMapping& operator=(MemoryMapping&& other) noexcept;

        MemoryMapping() = default;

        ~MemoryMapping();
    };

    MemoryMapping mapMemory();

    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceSize size{};
    std::shared_ptr<LogicalDevice> logicalDevice;
};

struct UniformPool {
    UniformPool() = default;

    UniformPool(const UniformPool&) = delete;

    UniformPool(UniformPool&&) = delete;

    UniformPool& operator=(const UniformPool&) = delete;

    UniformPool& operator=(UniformPool&&) = delete;

    ~UniformPool();

    void generateBuffers(size_t uniformSize);

    vk::DescriptorPool pool;
    std::vector<vk::DescriptorSet> sets;
    std::shared_ptr<LogicalDevice> logicalDevice;
    std::vector<std::shared_ptr<Buffer>> uniformBuffers;
    std::vector<Buffer::MemoryMapping> uniformMappings;

    struct UniformData {
        std::shared_ptr<Buffer> buffer;
        Buffer::MemoryMapping* mapping;
    };

    inline UniformData getUniformData(size_t set, size_t index = 0) {
        return {uniformBuffers[set * uniformCount + index], &uniformMappings[set * uniformCount + index]};
    }

    inline vk::DescriptorSet getSet(size_t set) {
        return sets[set];
    }

    void bindSampler(const std::shared_ptr<Sampler>& sampler, const std::shared_ptr<Image>& image, size_t setIndex, size_t samplerIndex, size_t binding, size_t arrayElement = 0);

    uint32_t uniformCount{};
    uint32_t samplerCount{};

private:
    void bindBuffer(const std::shared_ptr<Buffer>& buffer, size_t setIndex, size_t buffer_offset, size_t buffer_size,
                    size_t binding, size_t arrayElement = 0, size_t count = 1);
};

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

struct CommandPool {
    CommandPool() = default;

    CommandPool(const CommandPool&) = delete;

    CommandPool(CommandPool&&) = delete;

    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool& operator=(CommandPool&&) = delete;

    ~CommandPool();

    CommandBuffer createCommandBuffer();

    void freeCommandBuffer(CommandBuffer& commandBuffer);

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

    void deleteFramebuffers();

    void generateFramebuffers();

    void regenerateFramebuffer(size_t i);

    inline vk::Framebuffer& getFramebuffer(size_t i) {
        return framebuffers[i];
    }

    inline const vk::Framebuffer& getFramebuffer(size_t i) const {
        return framebuffers[i];
    }

    std::shared_ptr<LogicalDevice> logicalDevice;
    std::vector<vk::Framebuffer> framebuffers;
    std::shared_ptr<SwapChain> swapChain{};
    std::shared_ptr<Pipeline> pipeline{};
    bool generated{false};
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
    vk::DescriptorSetLayout descriptorSetLayout;
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

    void transferBuffer(const std::shared_ptr<Buffer>& from, const std::shared_ptr<Buffer>& to,
                        CommandBuffer& buffer, const std::shared_ptr<Fence>& fence) const;

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

struct ShaderDescriptor {
    enum Type {
        Vertex,
        Fragment,
        Geometry,
        TessellationControl,
        TessellationEvaluation,
        Compute
    };
    std::shared_ptr<ShaderModule> shader;
    Type type;
    std::string entryPoint = "main";
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

    std::shared_ptr<SwapChain>
    createSwapChain(const std::shared_ptr<Surface>& surface, const SwapChainInfo& info, vk::Extent2D extent,
                    vk::PresentModeKHR presentMode, vk::SurfaceFormatKHR surfaceFormat, uint32_t bufferCount);

    std::shared_ptr<ShaderModule> createShaderModule(const std::vector<char>& bytecode);

    std::shared_ptr<Pipeline> createPipeline(std::span<ShaderDescriptor> shaders,
                                             vk::PrimitiveTopology topology,
                                             vk::Format format,
                                             std::span<const VertexInputAttributeDescription> attributeDescription,
                                             std::span<const VertexInputBindingDescription> bindingDescription,
                                             std::span<const UniformDescription> uniformDescriptions);

    std::shared_ptr<Framebuffers> createFramebuffers(const std::shared_ptr<SwapChain>& swapChain,
                                                     const std::shared_ptr<Pipeline>& pipeline);

    std::shared_ptr<CommandPool> createCommandPool(const std::shared_ptr<Queue>& queue);

    std::shared_ptr<Fence> createFence(bool signaled = false);

    std::shared_ptr<Semaphore> createSemaphore();

    std::shared_ptr<Buffer> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    std::shared_ptr<Image> createImage(size_t width, size_t height, vk::Format format, vk::ImageTiling tiling,
                                       vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
    std::shared_ptr<Sampler> createSampler();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    std::shared_ptr<UniformPool> createUniformPool(uint32_t uniforms, uint32_t samplers, uint32_t poolSize, const std::shared_ptr<Pipeline>& pipeline);

    void waitIdle() const;

    void recreateSwapChain(std::shared_ptr<SwapChain>& swapChain, std::shared_ptr<Framebuffers>& framebuffers,
                           const std::shared_ptr<Surface>& surface, const SwapChainInfo& info, vk::Extent2D extent,
                           vk::PresentModeKHR presentMode, vk::SurfaceFormatKHR surfaceFormat,
                           uint32_t bufferCount);

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

    [[nodiscard]] std::shared_ptr<PhysicalDevice>
    createPhysicalDevice(uint32_t index, const std::shared_ptr<Surface>& surface) const;

    [[nodiscard]] std::shared_ptr<Surface> createSurface(const std::shared_ptr<Window>& window);


    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
};

void prepareCommandBuffer(const CommandBuffer& commandBuffer, const std::shared_ptr<Pipeline>& pipeline,
                          const std::shared_ptr<SwapChain>& swapChain, vk::Framebuffer& framebuffer,
                          size_t count, const std::span<std::shared_ptr<Buffer>>& vertexBuffers = {},
                          const std::shared_ptr<Buffer>& indexBuffer = nullptr,
                          vk::DescriptorSet descriptorSet = nullptr);

template<size_t InFlightCount = 1>
struct InFlightSwap {
    using recreateFunction = std::function<void()>;
    using requestCommandBufferFunction = std::function<std::span<CommandBuffer>(uint32_t count)>;
    using recordFunction = std::function<std::span<CommandBuffer>(requestCommandBufferFunction&, uint32_t imageIndex,
                                                                  std::shared_ptr<UniformPool>& uniformPool, uint32_t setIndex)>;

    InFlightSwap(std::shared_ptr<SwapChain> swapChain,
                 const std::shared_ptr<CommandPool>& pool,
                 const std::shared_ptr<Pipeline>& pipeline,
                 size_t uniformSize,
                 size_t uniformCount,
                 size_t samplerCount,
                 recordFunction recordCommandBufferCallback,
                 recreateFunction recreateSwapChainCallback) : swapChain(std::move(swapChain)),
                                                               recordCommandBufferCallback(
                                                                       std::move(recordCommandBufferCallback)),
                                                               recreateSwapChainCallback(
                                                                       std::move(recreateSwapChainCallback)),
                                                               pool(pool) {
        auto logicalDevice = InFlightSwap::swapChain->logicalDevice;
        uniformPool = logicalDevice->createUniformPool(uniformCount, samplerCount, InFlightCount, pipeline);
        if (uniformSize > 0) {
            uniformPool->generateBuffers(uniformSize);
        }
        for (int i = 0; i < InFlightCount; ++i) {
            inFlightFences.push_back(logicalDevice->createFence(true));
            imageAvailableSemaphores.push_back(logicalDevice->createSemaphore());
            renderFinishedSemaphores.push_back(logicalDevice->createSemaphore());
        }
        commandBuffers.resize(InFlightCount);
    }

    InFlightSwap(const InFlightSwap&) = delete;

    InFlightSwap(InFlightSwap&&) = delete;

    InFlightSwap& operator=(const InFlightSwap&) = delete;

    InFlightSwap& operator=(InFlightSwap&&) = delete;

    ~InFlightSwap() = default;

    void update(bool resized = false) {
        int index = currentFrame % InFlightCount;
        inFlightFences[index]->wait();
        auto image_opt = swapChain->acquireNextImage(imageAvailableSemaphores[index]);
        if (!image_opt) {
            recreateSwapChainCallback();
            return;
        }
        inFlightFences[index]->reset();
        requestCommandBufferFunction func = [index, this](uint32_t count) -> std::span<CommandBuffer> {
            auto& commandBufferArray = commandBuffers[index];
            if (commandBufferArray.size() < count) {
                auto dif = count - commandBufferArray.size();
                for (int i = 0; i < dif; ++i) {
                    commandBufferArray.push_back(pool->createCommandBuffer());
                }
            } else if (commandBufferArray.size() > count * 2) {
                auto dif = commandBufferArray.size() - count * 2;
                for (int i = 0; i < dif; ++i) {
                    auto command = commandBufferArray.back();
                    pool->freeCommandBuffer(command);
                    commandBufferArray.pop_back();
                }
            }
            return {commandBufferArray.data(), count};
        };
        auto resultCommandBuffers = recordCommandBufferCallback(func, *image_opt, uniformPool, index);
        for (auto command : resultCommandBuffers) {
            swapChain->logicalDevice->graphicsQueue->submit(command,
                                                            imageAvailableSemaphores[index],
                                                            renderFinishedSemaphores[index],
                                                            inFlightFences[index]);
        }
        auto success = swapChain->present(renderFinishedSemaphores[index], image_opt.value());
        if (!success || resized) {
            recreateSwapChainCallback();
        }
        currentFrame++;
    }

    std::shared_ptr<SwapChain> swapChain;

    std::vector<std::shared_ptr<Fence>> inFlightFences;
    std::vector<std::shared_ptr<Semaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<Semaphore>> renderFinishedSemaphores;
    std::shared_ptr<UniformPool> uniformPool;
    std::shared_ptr<CommandPool> pool;
    std::vector<std::vector<CommandBuffer>> commandBuffers;
    recreateFunction recreateSwapChainCallback;
    recordFunction recordCommandBufferCallback;
    int currentFrame = 0;
};

vk::DescriptorSetLayoutBinding createSamplerLayoutBinding(uint32_t binding);

}// namespace cr::vulkan