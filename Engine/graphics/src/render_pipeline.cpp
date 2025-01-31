#include "asset_manager.hpp"
#include "defines.hpp"
#include "extra.hpp"
#include "logger.hpp"
#include <model.hpp>
#include <render_pipeline.hpp>

using namespace Humongous::Utils;

namespace Humongous
{
RenderPipeline::RenderPipeline(LogicalDevice& logicalDevice, const RenderPipeline::PipelineConfigInfo& configinfo) : m_logicalDevice{logicalDevice}
{
    CreateRenderPipeline(configinfo);
}

RenderPipeline::~RenderPipeline()
{
    vkDestroyPipeline(m_logicalDevice.GetVkDevice(), m_pipeline, nullptr);
    HGINFO("Destroyed Render Pipeline");
}

void RenderPipeline::CreateRenderPipeline(const RenderPipeline::PipelineConfigInfo& configInfo)
{
    HGINFO("Creating Render Pipeline...");
    HGINFO("Reading shader files...");
    auto vertCode = ReadFile(configInfo.vertShaderPath);
    auto fragCode = ReadFile(configInfo.fragShaderPath);
    HGINFO("Successfully read shader files");

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    CreateShaderModule(vertCode, &vertShaderModule);
    CreateShaderModule(fragCode, &fragShaderModule);

    HGINFO("Successfully created shader modules");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.pNext = nullptr;

    if(!configInfo.bindless)
    {
        if(configInfo.inputBindings.size() == 0)
        {
            HGERROR("Trying to make a non-bindless render pipeline, but no vertex input bindings were specified!");
        }
        if(configInfo.attribBindings.size() == 0)
        {
            HGERROR("Trying to make a non-bindless render pipeline, but no vertex attribute bindings were specified!");
        }

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<n32>(configInfo.inputBindings.size());
        vertexInputInfo.pVertexBindingDescriptions = configInfo.inputBindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<n32>(configInfo.attribBindings.size());
        vertexInputInfo.pVertexAttributeDescriptions = configInfo.attribBindings.data();
    }

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &configInfo.renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

    pipelineInfo.layout = configInfo.pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    // pipelineInfo.subpass = configInfo.subpass;

    if(vkCreateGraphicsPipelines(m_logicalDevice.GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        HGERROR("Failed to create graphics pipeline!");
    }

    HGINFO("Successfully created graphics pipeline");

    vkDestroyShaderModule(m_logicalDevice.GetVkDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(m_logicalDevice.GetVkDevice(), fragShaderModule, nullptr);

    HGINFO("Successfully destroyed shader modules");
}

void RenderPipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const n32*>(code.data());

    if(vkCreateShaderModule(m_logicalDevice.GetVkDevice(), &createInfo, nullptr, shaderModule) != VK_SUCCESS)
    {
        HGERROR("Failed to create shader module!");
    }
}

RenderPipeline::PipelineConfigInfo RenderPipeline::DefaultPipelineConfigInfo()
{
    using namespace Systems;

    PipelineConfigInfo configInfo{.vertShaderPath = AssetManager::GetAsset(AssetManager::AssetType::SHADER, "simple.vert"),
                                  .fragShaderPath = AssetManager::GetAsset(AssetManager::AssetType::SHADER, "unlit.frag"),
                                  .bindless = true};

    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
    configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
    configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
    configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

    configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.front = {}; // Optional
    configInfo.depthStencilInfo.back = {};  // Optional

    configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount = static_cast<n32>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.flags = 0;

    configInfo.renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    configInfo.renderingInfo.viewMask = 0;
    configInfo.renderingInfo.colorAttachmentCount = 1;
    configInfo.renderingInfo.pColorAttachmentFormats = &configInfo.colorAttachmentFormat;
    configInfo.renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    // configInfo.renderingInfo.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    // hardcoded for now
    configInfo.colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    configInfo.renderingInfo.pColorAttachmentFormats = &configInfo.colorAttachmentFormat;
    configInfo.renderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    // configInfo.renderingInfo.stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    return configInfo;
}

} // namespace Humongous
