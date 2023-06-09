//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"

namespace cr::vulkan {

std::shared_ptr<UniformPool> LogicalDevice::createUniformPool(uint32_t uniforms, uint32_t samplers, uint32_t poolSize, const std::shared_ptr<Pipeline>& pipeline) {
    auto pool = std::make_shared<UniformPool>();
    pool->uniformCount = uniforms;
    pool->samplerCount = samplers;
    std::vector<vk::DescriptorPoolSize> poolSizeInfo;
    if (uniforms > 0) {
        poolSizeInfo.emplace_back(vk::DescriptorType::eUniformBuffer, poolSize * uniforms);
    }
    if (samplers > 0) {
        poolSizeInfo.emplace_back(vk::DescriptorType::eCombinedImageSampler, poolSize * samplers);
    }
    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = poolSizeInfo.size();
    poolInfo.pPoolSizes = poolSizeInfo.data();
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

//----------------------------------------------------------------------

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
    for (size_t setIndex = 0; setIndex < sets.size(); ++setIndex) {
        for (size_t j = 0; j < uniformCount; ++j) {
            auto buffer = logicalDevice->createBuffer(uniformSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                      vk::MemoryPropertyFlagBits::eHostVisible |
                                                              vk::MemoryPropertyFlagBits::eHostCoherent);
            uniformBuffers.push_back(buffer);
            uniformMappings.push_back(buffer->mapMemory());
            bindBuffer(buffer, setIndex, 0, uniformSize, 0, 0, 1);
        }
    }
}

void UniformPool::bindSampler(const std::shared_ptr<Sampler>& sampler, const std::shared_ptr<Image>& image, size_t setIndex, size_t samplerIndex, size_t binding, size_t arrayElement) {
    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = image->imageView;
    imageInfo.sampler = sampler->sampler;
    vk::WriteDescriptorSet descriptorWrite;
    descriptorWrite.dstSet = sets[setIndex];
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    logicalDevice->device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

}// namespace cr::vulkan