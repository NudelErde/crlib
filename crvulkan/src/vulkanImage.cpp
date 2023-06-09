//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"

namespace cr::vulkan {

std::shared_ptr<Image> LogicalDevice::createImage(size_t width, size_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
    auto image = std::make_shared<Image>();
    image->logicalDevice = shared_from_this();

    image->width = width;
    image->height = height;
    image->format = format;

    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.flags = vk::ImageCreateFlags();
    image->image = device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image->image);
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    image->memory = device.allocateMemory(allocInfo);
    device.bindImageMemory(image->image, image->memory, 0);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image->image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    image->imageView = device.createImageView(viewInfo);

    return image;
}

//----------------------------------------------------------------------

Image::~Image() {
    if (imageView) logicalDevice->device.destroyImageView(imageView);
    if (image) logicalDevice->device.destroyImage(image);
    if (memory) logicalDevice->device.freeMemory(memory);
}

std::shared_ptr<StagingBufferUpload> Image::upload(const std::span<const std::byte>& data, const std::shared_ptr<CommandPool>& commandPool) {
    auto upload = std::make_shared<StagingBufferUpload>();

    auto commandBuffer = commandPool->createCommandBuffer();
    commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    upload->commandBuffer = commandBuffer;
    upload->commandPool = commandPool;
    transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, commandBuffer);

    auto stagingBuffer = logicalDevice->createBuffer(data.size(), vk::BufferUsageFlagBits::eTransferSrc,
                                                     vk::MemoryPropertyFlagBits::eHostVisible |
                                                             vk::MemoryPropertyFlagBits::eHostCoherent);
    stagingBuffer->copyToBuffer(data);
    upload->stagingBuffer = stagingBuffer;

    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    commandBuffer.copyBufferToImage(stagingBuffer->buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, commandBuffer);

    upload->fence = logicalDevice->createFence();

    commandBuffer.end();

    auto submitInfo = vk::SubmitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    auto res = logicalDevice->device.resetFences(1, &upload->fence->fence);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to reset fence");

    res = commandPool->queue->queue.submit(1, &submitInfo, upload->fence->fence);
    if (res != vk::Result::eSuccess) throw std::runtime_error("failed to submit command buffer");

    return upload;
}

void Image::transitionLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout, CommandBuffer& commandBuffer) const {
    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage,
                                  vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
}
}// namespace cr::vulkan