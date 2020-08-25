#include "../../Head/7_IndexBuffer/CIndexBuffer.h"

namespace vk_Demo
{
	CIndexBuffer::~CIndexBuffer()
	{
		if (m_IndexBuffer) {
			delete m_IndexBuffer;
		}
		m_IndexBuffer = nullptr;
	}


	void CIndexBuffer::bind(VkCommandBuffer cmdBuffer)
	{
		vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer->getBuffer(), 0, m_IndexType);
	}

	static CIndexBuffer* CIndexBuffer::createindexBuffer(std::shared_ptr<CLogicalDevice> vulkanDevice, CCommandBuffer* cmdBuffer, std::vector<uint16_t> indices)
	{
		VkDevice device = vulkanDevice->getInstanceHandle();

		CIndexBuffer* indexBuffer = new CIndexBuffer();
		indexBuffer->m_Device = device;
		indexBuffer->m_IndexCount = indices.size();
		indexBuffer->m_IndexType = VK_INDEX_TYPE_UINT32;

		CBuffer* indexStaging = CBuffer::createBuffer(
			vulkanDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indices.size() * sizeof(uint32_t),
			indices.data()
		);

		indexBuffer->m_IndexBuffer = CBuffer::createBuffer(
			vulkanDevice,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indices.size() * sizeof(uint32_t)
		);

		cmdBuffer->begin();

		VkBufferCopy copyRegion = {};
		copyRegion.size = indices.size() * sizeof(uint32);

		vkCmdCopyBuffer(cmdBuffer->getCommandBuffer(), indexStaging->getBuffer(), indexBuffer->m_IndexBuffer->getBuffer(), 1, &copyRegion);

		cmdBuffer->end();
		cmdBuffer->submit();

		delete indexStaging;

		return indexBuffer;
	}

}