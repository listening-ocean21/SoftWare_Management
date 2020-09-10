#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	//pipeline��״̬�ļ��ϣ������ö�Ӧ��״̬
	struct SPipeLineCreateInfo 
	{
		VkPipelineInputAssemblyStateCreateInfo  PipelineInputAssemblyState;
		VkPipelineRasterizationStateCreateInfo  PipelineRasterizationState;
		VkPipelineColorBlendAttachmentState     PipelineColorBlendAttachmentState[8];
		VkPipelineDepthStencilStateCreateInfo   DepthStencilState;
		VkPipelineMultisampleStateCreateInfo    PipelineMultisampleState;
		VkPipelineTessellationStateCreateInfo   PipeLineTessellationState;

		//��ɫ��������VkShaderModule����ֻ���ֽ��뻺������һ����װ��������ɫ����û�б˴�����
		//ͨ��VkPipelineShaderStageCreateInfo�ṹ����ɫ��ģ����䵽�����еĶ������Ƭ����ɫ���׶�
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
			//ͼԪ���˽ṹ
			PipelineInputAssemblyState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			PipelineInputAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  //���㹲�棬���㲻����
			PipelineInputAssemblyState.primitiveRestartEnable = VK_FALSE;

			//��դ��
			PipelineRasterizationState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;;
			PipelineRasterizationState.polygonMode             = VK_POLYGON_MODE_FILL;  //�������β���ͼƬ�����ݣ���������
			PipelineRasterizationState.cullMode                = VK_CULL_MODE_BACK_BIT;    //ָ������������ͷ�ʽ
			PipelineRasterizationState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; //front-facing��Ķ����˳�򣬿�����˳ʱ��Ҳ��������ʱ��
			PipelineRasterizationState.depthClampEnable        = VK_FALSE;         //vK_TRUE:����Զ���ü����ƬԪ��������������������
			PipelineRasterizationState.rasterizerDiscardEnable = VK_FALSE;  //VK_TRUE������ͼԪ��Զ���ᴫ�ݵ���դ��
			PipelineRasterizationState.depthBiasEnable         = VK_FALSE;
			PipelineRasterizationState.depthBiasConstantFactor = 0.0f;
			PipelineRasterizationState.depthBiasClamp          = 0.0f;
			PipelineRasterizationState.depthBiasSlopeFactor    = 0.0f;
			PipelineRasterizationState.lineWidth               = 1.0f;                    //����ƬԪ�����������ߵĿ�ȡ������߿�֧��ȡ����Ӳ�����κδ���1.0���߿���Ҫ����GPU��wideLines����֧��

			//��ɫ���
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

			//��Ȳ���
			DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilState.depthTestEnable   = VK_TRUE;    //ָ���Ƿ�Ӧ�ý��µ���Ȼ���������Ȼ��������бȽϣ���ȷ���Ƿ�Ӧ�ñ�����
			DepthStencilState.depthWriteEnable  = VK_TRUE;   //ָ��ͨ����Ȳ��Ե��µ�Ƭ������Ƿ�Ӧ�ñ�ʵ��д����Ȼ�����
			DepthStencilState.stencilTestEnable = VK_TRUE;
			DepthStencilState.depthCompareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;  //ָ��ִ�б������߶���Ƭ�εıȽ�ϸ��
			DepthStencilState.back.failOp       = VK_STENCIL_OP_KEEP;
			DepthStencilState.back.passOp       = VK_STENCIL_OP_KEEP;
			DepthStencilState.back.compareOp    = VK_COMPARE_OP_ALWAYS;
			DepthStencilState.front             = DepthStencilState.back;
	
			//���ز���
			PipelineMultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			PipelineMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			PipelineMultisampleState.pSampleMask = nullptr; // Optional

			//����ϸ��
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