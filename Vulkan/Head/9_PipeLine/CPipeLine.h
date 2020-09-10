#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	//pipeline是状态的集合，并设置对应的状态
	struct SPipeLineCreateInfo 
	{
		VkPipelineInputAssemblyStateCreateInfo  PipelineInputAssemblyState;
		VkPipelineRasterizationStateCreateInfo  PipelineRasterizationState;
		VkPipelineColorBlendAttachmentState     PipelineColorBlendAttachmentState[8];
		VkPipelineDepthStencilStateCreateInfo   DepthStencilState;
		VkPipelineMultisampleStateCreateInfo    PipelineMultisampleState;
		VkPipelineTessellationStateCreateInfo   PipeLineTessellationState;

		//着色器创建：VkShaderModule对象只是字节码缓冲区的一个包装容器。着色器并没有彼此链接
		//通过VkPipelineShaderStageCreateInfo结构将着色器模块分配到管线中的顶点或者片段着色器阶段
		VkShaderModule	vertShaderModule = VK_NULL_HANDLE;
		VkShaderModule	fragShaderModule = VK_NULL_HANDLE;
		VkShaderModule	compShaderModule = VK_NULL_HANDLE;
		VkShaderModule	tescShaderModule = VK_NULL_HANDLE;
		VkShaderModule	teseShaderModule = VK_NULL_HANDLE;
		VkShaderModule	geomShaderModule = VK_NULL_HANDLE;

		VkShaderModule*		    shader = nullptr;
		uint32_t			subpass = 0;
		uint32_t            colorAttachmentCount = 1;

		SPipeLineCreateInfo()
		{
			//图元拓扑结构
			PipelineInputAssemblyState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			PipelineInputAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  //三点共面，顶点不共用
			PipelineInputAssemblyState.primitiveRestartEnable = VK_FALSE;

			//光栅化
			PipelineRasterizationState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;;
			PipelineRasterizationState.polygonMode             = VK_POLYGON_MODE_FILL;  //决定几何产生图片的内容，多边形填充
			PipelineRasterizationState.cullMode                = VK_CULL_MODE_BACK_BIT;    //指定剪裁面的类型方式
			PipelineRasterizationState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; //front-facing面的顶点的顺序，可以是顺时针也可以是逆时针
			PipelineRasterizationState.depthClampEnable        = VK_FALSE;         //vK_TRUE:超过远近裁剪面的片元进行收敛，而不是舍弃
			PipelineRasterizationState.rasterizerDiscardEnable = VK_FALSE;  //VK_TRUE：几何图元永远不会传递到光栅化
			PipelineRasterizationState.depthBiasEnable         = VK_FALSE;
			PipelineRasterizationState.depthBiasConstantFactor = 0.0f;
			PipelineRasterizationState.depthBiasClamp          = 0.0f;
			PipelineRasterizationState.depthBiasSlopeFactor    = 0.0f;
			PipelineRasterizationState.lineWidth               = 1.0f;                    //根据片元的数量描述线的宽度。最大的线宽支持取决于硬件，任何大于1.0的线宽需要开启GPU的wideLines特性支持

			//颜色混合
			for (uint32_t i = 0; i < 8; i++) 
			{
				PipelineColorBlendAttachmentState[i]                     = {};
				PipelineColorBlendAttachmentState[i].blendEnable         = VK_FALSE;
				PipelineColorBlendAttachmentState[i].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				PipelineColorBlendAttachmentState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				PipelineColorBlendAttachmentState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				PipelineColorBlendAttachmentState[i].colorBlendOp        = VK_BLEND_OP_ADD; // Optional
				PipelineColorBlendAttachmentState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
				PipelineColorBlendAttachmentState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
				PipelineColorBlendAttachmentState[i].alphaBlendOp        = VK_BLEND_OP_ADD; // Optional
			}

			//深度测试
			DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilState.depthTestEnable   = VK_TRUE;    //指定是否应该将新的深度缓冲区与深度缓冲区进行比较，以确认是否应该被丢弃
			DepthStencilState.depthWriteEnable  = VK_TRUE;   //指定通过深度测试的新的片段深度是否应该被实际写入深度缓冲区
			DepthStencilState.stencilTestEnable = VK_TRUE;
			DepthStencilState.depthCompareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;  //指定执行保留或者丢弃片段的比较细节
			DepthStencilState.back.failOp       = VK_STENCIL_OP_KEEP;
			DepthStencilState.back.passOp       = VK_STENCIL_OP_KEEP;
			DepthStencilState.back.compareOp    = VK_COMPARE_OP_ALWAYS;
			DepthStencilState.front             = DepthStencilState.back;
	
			//多重采样
			PipelineMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			PipelineMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			PipelineMultisampleState.pSampleMask = nullptr; // Optional

			//曲面细分
			PipeLineTessellationState.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			PipeLineTessellationState.patchControlPoints = 0;
		}


		void fillShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages)
		{
			if (vertShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				shaderStageCreateInfo.module = vertShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}

			if (fragShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				shaderStageCreateInfo.module = fragShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}

			if (compShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				shaderStageCreateInfo.module = compShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}

			if (geomShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				shaderStageCreateInfo.module = geomShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}

			if (tescShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				shaderStageCreateInfo.module = tescShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}

			if (teseShaderModule != VK_NULL_HANDLE)
			{
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				shaderStageCreateInfo.module = teseShaderModule;
				shaderStageCreateInfo.pName = "main";
				shaderStages.push_back(shaderStageCreateInfo);
			}
		}
	};
	class CPipeLine
	{
	public:
		CPipeLine() : m_Device(nullptr), m_PipeLine(VK_NULL_HANDLE) {};
		~CPipeLine();

		CPipeLine* createPipeLine(
			std::shared_ptr<CLogicalDevice> vulkanDevice,
			VkPipelineCache pipelineCache,
			SPipeLineCreateInfo& pipelineInfo,
			const std::vector<VkVertexInputBindingDescription>& inputBindings,
			const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributs,
			VkPipelineLayout pipelineLayout,
			VkRenderPass renderPass);

	private:
	    std::shared_ptr<CLogicalDevice> m_Device;
		VkPipeline			m_PipeLine;
		VkPipelineLayout	m_PipelineLayout;
	};
}