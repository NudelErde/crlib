//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"

namespace cr::vulkan {

std::shared_ptr<Sampler> LogicalDevice::createSampler() {
    auto sampler = std::make_shared<Sampler>();
    sampler->logicalDevice = shared_from_this();

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = false;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = 0;

    sampler->sampler = device.createSampler(samplerInfo);

    return sampler;
}

//----------------------------------------------------------------------

Sampler::~Sampler() {
    if (sampler) logicalDevice->device.destroySampler(sampler);
}

}// namespace cr::vulkan