//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"

namespace cr::vulkan {

//----------------------------------------------------------------------

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

}// namespace cr::vulkan