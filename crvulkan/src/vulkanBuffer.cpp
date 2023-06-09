//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"

namespace cr::vulkan {

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
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    auto m = device.allocateMemory(allocInfo);
    buffer->memory = m;
    device.bindBufferMemory(buffer->buffer, buffer->memory, 0);
    return buffer;
}

//----------------------------------------------------------------------

Buffer::~Buffer() {
    logicalDevice->device.destroyBuffer(buffer);
    logicalDevice->device.freeMemory(memory);
}

void Buffer::copyToBuffer(const std::span<const std::byte>& data) {
    void* ptr = nullptr;
    auto res = logicalDevice->device.mapMemory(memory, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(), &ptr);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to map memory");
    memcpy(ptr, data.data(), data.size());
    logicalDevice->device.flushMappedMemoryRanges(vk::MappedMemoryRange(memory, 0, VK_WHOLE_SIZE));
    logicalDevice->device.unmapMemory(memory);
}

std::shared_ptr<StagingBufferUpload> Buffer::copyToBufferUsingStagingBuffer(const std::span<const std::byte>& data, const std::shared_ptr<CommandPool>& commandPool) {
    auto stagingBuffer = logicalDevice->createBuffer(data.size(), vk::BufferUsageFlagBits::eTransferSrc,
                                                     vk::MemoryPropertyFlagBits::eHostVisible |
                                                             vk::MemoryPropertyFlagBits::eHostCoherent);
    stagingBuffer->copyToBuffer(data);
    auto upload = std::make_shared<StagingBufferUpload>();
    upload->stagingBuffer = stagingBuffer;
    upload->commandBuffer = commandPool->createCommandBuffer();
    upload->commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::BufferCopy copyRegion;
    copyRegion.size = data.size();
    upload->commandBuffer.copyBuffer(stagingBuffer->buffer, buffer, copyRegion);
    upload->commandBuffer.end();
    upload->commandPool = commandPool;
    upload->fence = logicalDevice->createFence();
    commandPool->queue->queue.submit({vk::SubmitInfo(0, nullptr, nullptr, 1, &upload->commandBuffer)}, upload->fence->fence);
    return upload;
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

//----------------------------------------------------------------------

StagingBufferUpload::~StagingBufferUpload() {
    wait();
    commandPool->freeCommandBuffer(commandBuffer);
}
void StagingBufferUpload::wait() {
    fence->wait();
}

void StagingBufferUpload::waitAll(const std::span<std::shared_ptr<cr::vulkan::StagingBufferUpload>>& uploads) {
    if (uploads.empty()) return;
    std::vector<vk::Fence> fences;
    for (auto& upload : uploads) {
        fences.push_back(upload->fence->fence);
    }
    auto res = uploads[0]->commandPool->logicalDevice->device.waitForFences(fences, true, UINT64_MAX);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to wait for fences");
}

}// namespace cr::vulkan