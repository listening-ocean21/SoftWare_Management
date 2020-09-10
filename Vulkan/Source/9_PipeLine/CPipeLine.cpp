#include"../../Head/9_PipeLine/CPipeLine.h"

namespace vk_Demo
{

	CPipeLine::~CPipeLine()
	{
		if (m_PipeLine != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device->getInstanceHandle(), m_PipeLine, nullptr);
		}
	}

	CPipeLine* CPipeLine::createPipeLine(
		std::shared_ptr<CLogicalDevice> vulkanDevice,
		VkPipelineCache pipelineCache,
		SPipeLineCreateInfo& pipelineInfo,
		const std::vector<VkVertexInputBindingDescription>& inputBindings,
		const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributs,
		VkPipelineLayout pipelineLayout,
		VkRenderPass renderPass)
	{
		CPipeLine* pipeline = new CPipeLine();
		pipeline->m_Device = vulkanDevice;
		pipeline->m_PipelineLayout = pipelineLayout;

		VkDevice device = vulkanDevice->getInstanceHandle();

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = inputBindings.size();
		vertexInputState.pVertexBindingDescriptions = inputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = vertexInputAttributs.size();
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

		VkPipelineColorBlendStateCreateInfo colorBlendState;
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = pipelineInfo.colorAttachmentCount;
		colorBlendState.pAttachments = pipelineInfo.PipelineColorBlendAttachmentState;

		VkPipelineViewportStateCreateInfo viewportState;
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dynamicState;
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStateEnables.data();

		//通过VkPipelineShaderStageCreateInfo结构将着色器模块分配到管线中的顶点或者片段着色器阶段
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		if (pipelineInfo.shader) {
			shaderStages = pipelineInfo.shader->shaderStageCreateInfos;
		}
		else {
			pipelineInfo.fillShaderStages(shaderStages);
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout              = pipelineLayout;
		pipelineCreateInfo.renderPass          = renderPass;
		pipelineCreateInfo.subpass             = pipelineInfo.subpass;
		pipelineCreateInfo.stageCount          = shaderStages.size();
		pipelineCreateInfo.pStages             = shaderStages.data();
		pipelineCreateInfo.pVertexInputState   = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &(pipelineInfo.PipelineInputAssemblyState);
		pipelineCreateInfo.pRasterizationState = &(pipelineInfo.PipelineRasterizationState);
		pipelineCreateInfo.pColorBlendState    = &colorBlendState;
		pipelineCreateInfo.pMultisampleState   = &(pipelineInfo.PipelineMultisampleState);
		pipelineCreateInfo.pViewportState      = &viewportState;
		pipelineCreateInfo.pDepthStencilState  = &(pipelineInfo.DepthStencilState);
		pipelineCreateInfo.pDynamicState       = &dynamicState;

		if (pipelineInfo.PipeLineTessellationState.patchControlPoints != 0) {
			pipelineCreateInfo.pTessellationState = &(pipelineInfo.PipeLineTessellationState);
		}

		vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &(pipeline->m_PipeLine));

		return pipeline;
	}
}