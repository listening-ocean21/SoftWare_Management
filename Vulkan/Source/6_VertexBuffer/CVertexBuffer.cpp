#include"../../Head/6_VertexBuffer/CVertexBuffer.h"

namespace vk_Demo
{
	CVertexBuffer::CVertexBuffer() {};
	CVertexBuffer::~CVertexBuffer()
	{
		if (m_VertexBuffer) {
			delete m_VertexBuffer;
		}
		m_VertexBuffer = nullptr;
	}
	CVertexBuffer* CVertexBuffer::createVertexBuffer(std::shared_ptr<CLogicalDevice> vulkanDevice, CCommandBuffer* cmdBuffer, std::vector<float> vertices, const std::vector<EVertexAttribute>& attributes)
	{
		VkDevice device = vulkanDevice->getInstanceHandle();

		CVertexBuffer* vertexBuffer = new CVertexBuffer();
		vertexBuffer->m_Device = device;
		vertexBuffer->m_Attributes = attributes;

		CBuffer* vertexStaging = CBuffer::createBuffer(
			vulkanDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertices.size() * sizeof(float),
			vertices.data()
		);

		vertexBuffer->m_VertexBuffer = CBuffer::createBuffer(
			vulkanDevice,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertices.size() * sizeof(float),
			vertices.data()
		);

		//Â¼ÖÆÃüÁî
		cmdBuffer->begin();

		VkBufferCopy copyRegion = {};
		copyRegion.size = vertices.size() * sizeof(float);
		vkCmdCopyBuffer(cmdBuffer->m_CommandBuffer, vertexStaging->getBuffer(), vertexBuffer->m_VertexBuffer->getBuffer(), 1, &copyRegion);

		cmdBuffer->end();
		cmdBuffer->submit();

		delete vertexStaging;

		return vertexBuffer;
	}

	VkVertexInputBindingDescription CVertexBuffer::getInputBinding()
	{
		uint32_t stride = 0;
		for (uint32_t i = 0; i < m_Attributes.size(); ++i) {
			stride += __vertexAttributeToSize(m_Attributes[i]);
		}

		VkVertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = stride;
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertexInputBinding;
	}
	std::vector<VkVertexInputAttributeDescription> CVertexBuffer::getInputAttributes(const std::vector<EVertexAttribute>& shaderInputs)
	{
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributs;
		uint32_t offset = 0;
		for (uint32_t i = 0; i < shaderInputs.size(); ++i)
		{
			VkVertexInputAttributeDescription inputAttribute = {};
			inputAttribute.binding = 0;
			inputAttribute.location = i;
			inputAttribute.format = __vertexAttributeToVkFormat(shaderInputs[i]);
			inputAttribute.offset = offset;
			offset += __vertexAttributeToSize(shaderInputs[i]);
			vertexInputAttributs.push_back(inputAttribute);
		}
		return vertexInputAttributs;
	}
	void CVertexBuffer::bind(VkCommandBuffer cmdBuffer)
	{
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &(m_VertexBuffer->getHandle()), &m_Offset);
	}
}