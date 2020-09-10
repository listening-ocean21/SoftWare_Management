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


	void CIndexBuffer::bindDraw(VkCommandBuffer cmdBuffer)
	{
		vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer->getHandle(), 0, m_IndexType);
		vkCmdDrawIndexed(cmdBuffer, m_IndexCount, 1, 0, 0, 0);
	}

	CIndexBuffer* CIndexBuffer::createIndexBuffer(std::shared_ptr<CLogicalDevice> vDevice, CCommandBuffer* cmdBuffer, std::vector<uint16_t> indices)
	{
		VkDevice device = vDevice->getInstanceHandle();

		CIndexBuffer* indexBuffer = new CIndexBuffer();
		indexBuffer->m_Device = device;
		indexBuffer->m_IndexCount = indices.size();
		indexBuffer->m_IndexType = VK_INDEX_TYPE_UINT32;

		CBuffer* indexStaging = CBuffer::createBuffer(
			vDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indices.size() * sizeof(uint16_t),
			indices.data()
		);

		indexBuffer->m_IndexBuffer = CBuffer::createBuffer(
			vDevice,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indices.size() * sizeof(uint16_t)
		);

		cmdBuffer->begin();

		VkBufferCopy copyRegion = {};
		copyRegion.size = indices.size() * sizeof(uint16_t);

		vkCmdCopyBuffer(cmdBuffer->getCommandBuffer(), indexStaging->getHandle(), indexBuffer->m_IndexBuffer->getHandle(), 1, &copyRegion);

		cmdBuffer->end();
		cmdBuffer->submit();

		delete indexStaging;

		return indexBuffer;
	}

	CIndexBuffer* CIndexBuffer::createIndexBuffer(std::shared_ptr<CLogicalDevice> vDevice, CCommandBuffer* cmdBuffer, std::vector<uint32_t> indices)
	{
		VkDevice device = vDevice->getInstanceHandle();

		CIndexBuffer* indexBuffer = new CIndexBuffer();
		indexBuffer->m_Device = device;
		indexBuffer->m_IndexCount = indices.size();
		indexBuffer->m_IndexType = VK_INDEX_TYPE_UINT32;

		CBuffer* indexStaging = CBuffer::createBuffer(
			vDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indices.size() * sizeof(uint32_t),
			indices.data()
		);

		indexBuffer->m_IndexBuffer = CBuffer::createBuffer(
			vDevice,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indices.size() * sizeof(uint32_t)
		);

		cmdBuffer->begin();

		VkBufferCopy copyRegion = {};
		copyRegion.size = indices.size() * sizeof(uint32_t);

		vkCmdCopyBuffer(cmdBuffer->getCommandBuffer(), indexStaging->getHandle(), indexBuffer->m_IndexBuffer->getHandle(), 1, &copyRegion);

		cmdBuffer->end();
		cmdBuffer->submit();

		delete indexStaging;

		return indexBuffer;
	}
}