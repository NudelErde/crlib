//
// Created by nudelerde on 09.06.23.
//

#include "vulkan.h"
namespace cr::vulkan {
std::shared_ptr<Pipeline> LogicalDevice::createPipeline(std::span<ShaderDescriptor> shaders,
                                                        vk::PrimitiveTopology topology,
                                                        vk::Format format,
                                                        std::span<const VertexInputAttributeDescription> attributeDescription,
                                                        std::span<const VertexInputBindingDescription> bindingDescription,
                                                        std::span<const UniformDescription> uniformDescriptions) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->logicalDevice = shared_from_this();

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    for (auto& shader : shaders) {
        vk::PipelineShaderStageCreateInfo shaderStageInfo;
        shaderStageInfo.module = shader.shader->shaderModule;
        shaderStageInfo.pName = shader.entryPoint.c_str();
        switch (shader.type) {
            case ShaderDescriptor::Vertex:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
                break;
            case ShaderDescriptor::Fragment:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
                break;
            case ShaderDescriptor::Geometry:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eGeometry;
                break;
            case ShaderDescriptor::TessellationControl:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eTessellationControl;
                break;
            case ShaderDescriptor::TessellationEvaluation:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eTessellationEvaluation;
                break;
            case ShaderDescriptor::Compute:
                shaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
                break;
            default:
                throw std::runtime_error("Unknown shader type");
        }
        shaderStages.push_back(shaderStageInfo);
    }

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
    colorAttachment.format = format;
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
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
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
}// namespace cr::vulkan