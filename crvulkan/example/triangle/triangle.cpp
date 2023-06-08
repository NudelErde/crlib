//
// Created by nudelerde on 05.06.23.
//

constexpr auto WIDTH = 800;
constexpr auto HEIGHT = 600;
constexpr auto MAX_FRAMES_IN_FLIGHT = 2;

#include "crmath/geometry.h"
#include "crmath/matrix.h"
#include "crvulkan/vulkan.h"
#include "crvulkan/window.h"
#include <chrono>
#include <fstream>

struct Uniform {
    cr::math::matrix<float, 4, 4> model;
    cr::math::matrix<float, 4, 4> view;
    cr::math::matrix<float, 4, 4> proj;
};
static_assert(sizeof(Uniform) == sizeof(float) * 16 * 3);

struct Vertex {
    cr::math::cvector<float, 2> pos;
    cr::math::cvector<float, 3> color;

    static cr::vulkan::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<cr::vulkan::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        return {
                vk::VertexInputAttributeDescription{0, 0, cr::vulkan::VertexAttributeFormat::eFloatVec2, offsetof(Vertex, pos)},
                vk::VertexInputAttributeDescription{1, 0, cr::vulkan::VertexAttributeFormat::eFloatVec3, offsetof(Vertex, color)}};
    }
};
static_assert(sizeof(Vertex) == sizeof(float) * 5);

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

    auto attributeDescription = Vertex::getAttributeDescriptions();
    auto bindingDescription = std::array<cr::vulkan::VertexInputBindingDescription, 1>{Vertex::getBindingDescription()};

    std::array<vk::DescriptorSetLayoutBinding, 1> bindings{
            vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}};

    auto pipeline = logicalDevice->createPipeline(vert_shader_module, frag_shader_module,
                                                  vk::PrimitiveTopology::eTriangleList, swapChain,
                                                  attributeDescription, bindingDescription, bindings);
    auto framebuffers = logicalDevice->createFramebuffers(swapChain, pipeline);
    auto commandPool = logicalDevice->createCommandPool(logicalDevice->graphicsQueue);

    std::array<Vertex, 6> vertexData{
            Vertex{cr::math::cvector<float, 2>{-0.5f, -0.5f}, cr::math::cvector<float, 3>{1.0f, 0.0f, 0.0f}},
            Vertex{cr::math::cvector<float, 2>{0.5f, -0.5f}, cr::math::cvector<float, 3>{0.0f, 1.0f, 0.0f}},
            Vertex{cr::math::cvector<float, 2>{-0.5f, 0.5f}, cr::math::cvector<float, 3>{0.0f, 0.0f, 1.0f}},
            Vertex{cr::math::cvector<float, 2>{0.5f, 0.5f}, cr::math::cvector<float, 3>{1.0f, 0.0f, 0.0f}},
    };

    std::array<uint32_t, 6> indexData{
            0, 1, 2, 3, 2, 1};

    auto vertexBuffer = logicalDevice->createBuffer(sizeof(vertexData[0]) * vertexData.size(),
                                                    vk::BufferUsageFlagBits::eVertexBuffer,
                                                    vk::MemoryPropertyFlagBits::eHostVisible);

    auto indexBuffer = logicalDevice->createBuffer(sizeof(indexData[0]) * indexData.size(),
                                                   vk::BufferUsageFlagBits::eIndexBuffer,
                                                   vk::MemoryPropertyFlagBits::eHostVisible);

    vertexBuffer->copyToBuffer(std::span<std::byte>(reinterpret_cast<std::byte*>(vertexData.data()), vertexData.size() * sizeof(vertexData[0])));
    indexBuffer->copyToBuffer(std::span<std::byte>(reinterpret_cast<std::byte*>(indexData.data()), indexData.size() * sizeof(indexData[0])));

    std::array<std::shared_ptr<cr::vulkan::Buffer>, 1> vertexBuffers{vertexBuffer};

    auto start = std::chrono::steady_clock::now();

    auto recordBuffer = [&](cr::vulkan::CommandBuffer& buffer, uint32_t imageIndex, cr::vulkan::UniformPool::UniformData data) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        float time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() / 1000.0f;
        Uniform uniform{
                cr::math::scale_matrix<float>(std::sin(time), std::sin(time), 1.0f, 1.0f),
                cr::math::identity<float, 4>(),
                cr::math::identity<float, 4>()};
        data.mapping.as<Uniform>()[0] = uniform;
        buffer.reset();
        cr::vulkan::prepareCommandBuffer(buffer, pipeline, swapChain, framebuffers->getFramebuffer(imageIndex),
                                         indexData.size(), vertexBuffers, indexBuffer, data.set);
    };
    auto recreatePipeline = [&]() {
        swapChainInfo = physicalDevice->getSwapChainInfo();
        logicalDevice->recreateSwapChain(swapChain, framebuffers, surface, swapChainInfo,
                                         swapChainInfo.chooseExtent(window),
                                         swapChainInfo.choosePresentMode(),
                                         swapChainInfo.chooseSurfaceFormat(),
                                         swapChainInfo.capabilities.minImageCount + 1);
    };

    cr::vulkan::InFlightSwap<2> inFlightSwap(swapChain, commandPool,
                                             pipeline, sizeof(Uniform),
                                             recordBuffer, recreatePipeline);

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

        inFlightSwap.update(resized);
    }
    logicalDevice->waitIdle();
}
