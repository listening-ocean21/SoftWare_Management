#include <vector>

#include "../../Head/2_Buffer/CBuffer.h"

namespace vk_Demo
{
	CBuffer::~CBuffer()
	{
		if (m_Buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(m_Device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_DeviceMemory != VK_NULL_HANDLE) {
			vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
		}
	}

	//�������豸�ڴ��뱾����������ӳ���ϵ
	VkResult CBuffer::map(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		if (m_pMapped)
		{
			return VK_SUCCESS;
		}

		vkMapMemory(m_Device, m_DeviceMemory, vOffset, vSize, 0, &m_pMapped);
	}

	//���ӳ��
	void CBuffer::unMap()
	{
		if (!m_pMapped) return;

		vkUnmapMemory(m_Device, m_DeviceMemory);
		m_pMapped = nullptr;
	}

	//���豸�ڴ�󶨵�Buffer��
	VkResult CBuffer::bind(VkDeviceSize vOffset)
	{
		return vkBindBufferMemory(m_Device, m_Buffer, m_DeviceMemory, vOffset);
	}
	
	//�����ݸ��Ƶ�Buffer
	void CBuffer::copyToBuffer(void* vData, VkDeviceSize vSize)
	{
		_ASSERT(m_pMapped);
		memcpy(m_pMapped, vData, vSize);
	}

	//����Buffer����
	 CBuffer* CBuffer::createBuffer(std::shared_ptr<CLogicalDevice> vDevice, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vMemoryPropertyFlags, VkDeviceSize vSize, void* vData)
	{
		CBuffer* pBuffer = new CBuffer();

		VkDevice vkDevice = vDevice->getInstanceHandle();
		pBuffer->m_Device = vkDevice;

		//����������
		VkBufferCreateInfo BufferInfo = {};
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = vSize;
		BufferInfo.usage = vUsageFlags;                    //ָ�������е����ݵ�ʹ��Ŀ��
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//������ͼ��һ����������Ա��ض��Ķ�������ӵ�У�Ҳ����ͬʱ�ڶ��������֮ǰ��������Ϊ����ģʽ
		if (vkCreateBuffer(vkDevice, &BufferInfo, nullptr, &(pBuffer->m_Buffer)) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		//�鿴�ض����������ڴ�Ҫ��
		VkMemoryRequirements MemRequirements;
		vkGetBufferMemoryRequirements(vkDevice, pBuffer -> m_Buffer, &MemRequirements);

		//����ض������豸�ṩ���ڴ�����
		uint32_t MemoryTypeIndex = 0;
		VkMemoryAllocateInfo MemoryAllocateInfo = {};
		MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocateInfo.allocationSize = MemRequirements.size;
		//vDevice->GetMemoryManager().GetMemoryTypeFromProperties(MemRequirements.memoryTypeBits, vMemoryPropertyFlags, &MemoryTypeIndex);

		VkPhysicalDeviceMemoryProperties MemProperties;
		vkGetPhysicalDeviceMemoryProperties(vDevice->getPhysicalDevice(), &MemProperties);

		//TODO
		uint32_t temp;
		for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++) {
			if ((MemRequirements.memoryTypeBits & (1 << i)) && (MemProperties.memoryTypes[i].propertyFlags & vMemoryPropertyFlags) == vMemoryPropertyFlags) {
				temp = i ;
			}
		}
		MemoryAllocateInfo.memoryTypeIndex = temp;

		if (vkAllocateMemory(vkDevice, &MemoryAllocateInfo, nullptr, &pBuffer ->m_DeviceMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		pBuffer->m_BufferSize = MemoryAllocateInfo.allocationSize;
		pBuffer->m_Alignment = MemRequirements.alignment;
		pBuffer->m_BufferUsageFlags = vUsageFlags;
		pBuffer->m_MemoryPropertyFlags = vMemoryPropertyFlags;

		//�����ݸ��Ƶ�������
		if (vData != nullptr)
		{
			pBuffer->map();
			memcpy(pBuffer->m_pMapped,vData, vSize);
			//�������ڴ��������ڻ��棬���������������ݣ���Ҫ֪ͨ��������ʹ���ڴ�һ����֪ͨ
			if ((vMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				pBuffer->flush();
			}
			pBuffer->unMap();
		}

		pBuffer->bind();

		return pBuffer;
	}
	
	//�������򲻻�����֪�����ݸ��Ƶ�Buffer�Ѿ���ɣ����棩��Ҳ���ܳ���ӳ����ڴ��д����������ɼ�
	//���ֽ����ʽ��1.ʹ������һ�µ��ڴ�ѿռ䣬��VK_MEMORY_PROPERTY_HOST_COHERENT_BITָ��
	//2.�����д���ڴ�ӳ������󣬵���vkFlushMappedMemoryRanges����������ȡӳ���ڴ�֮ǰ������vkInvalidateMappedMemoryRanges����
	VkResult CBuffer::flush(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		VkMappedMemoryRange MappedRange = {};
		MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		MappedRange.memory = m_DeviceMemory;
		MappedRange.offset = vOffset;
		MappedRange.size = vSize;
		return vkFlushMappedMemoryRanges(m_Device, 1, &MappedRange);
	}

	//����ȡӳ���ڴ�֮ǰ������vkInvalidateMappedMemoryRanges����
	VkResult CBuffer::invalidate(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		VkMappedMemoryRange MappedRange = {};
		MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		MappedRange.memory = m_DeviceMemory;
		MappedRange.offset = vOffset;
		MappedRange.size = vSize;
		return vkInvalidateMappedMemoryRanges(m_Device, 1, &MappedRange);
	}

	//����������
	void CBuffer::setupDescriptore(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		m_BufferDescriptor.offset = vOffset;
		m_BufferDescriptor.buffer = m_Buffer;
		m_BufferDescriptor.range = vSize;
	}
}


