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

	//将物理设备内存与本地主机建立映射关系
	VkResult CBuffer::map(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		if (m_pMapped)
		{
			return VK_SUCCESS;
		}

		vkMapMemory(m_Device, m_DeviceMemory, vOffset, vSize, 0, &m_pMapped);
	}

	//解除映射
	void CBuffer::unMap()
	{
		if (!m_pMapped) return;

		vkUnmapMemory(m_Device, m_DeviceMemory);
		m_pMapped = nullptr;
	}

	//将设备内存绑定到Buffer上
	VkResult CBuffer::bind(VkDeviceSize vOffset)
	{
		return vkBindBufferMemory(m_Device, m_Buffer, m_DeviceMemory, vOffset);
	}
	
	//将数据复制到Buffer
	void CBuffer::copyToBuffer(void* vData, VkDeviceSize vSize)
	{
		_ASSERT(m_pMapped);
		memcpy(m_pMapped, vData, vSize);
	}

	//创建Buffer对象
	 CBuffer* CBuffer::createBuffer(std::shared_ptr<CLogicalDevice> vDevice, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vMemoryPropertyFlags, VkDeviceSize vSize, void* vData)
	{
		CBuffer* pBuffer = new CBuffer();

		VkDevice vkDevice = vDevice->getInstanceHandle();
		pBuffer->m_Device = vkDevice;

		//创建缓冲区
		VkBufferCreateInfo BufferInfo = {};
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = vSize;
		BufferInfo.usage = vUsageFlags;                    //指定缓冲中的数据的使用目的
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//交换链图像一样，缓冲可以被特定的队列族所拥有，也可以同时在多个队列族之前共享，这里为独享模式
		if (vkCreateBuffer(vkDevice, &BufferInfo, nullptr, &(pBuffer->m_Buffer)) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		//查看特定缓冲区的内存要求
		VkMemoryRequirements MemRequirements;
		vkGetBufferMemoryRequirements(vkDevice, pBuffer -> m_Buffer, &MemRequirements);

		//检查特定物理设备提供的内存属性
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

		//将数据复制到缓冲区
		if (vData != nullptr)
		{
			pBuffer->map();
			memcpy(pBuffer->m_pMapped,vData, vSize);
			//由于现在处理器存在缓存，不会立即处理数据，需要通知驱动程序，使用内存一致性通知
			if ((vMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				pBuffer->flush();
			}
			pBuffer->unMap();
		}

		pBuffer->bind();

		return pBuffer;
	}
	
	//驱动程序不会立即知道数据复制到Buffer已经完成（缓存），也可能尝试映射的内存对写缓冲操作不可见
	//两种解决方式：1.使用主机一致的内存堆空间，用VK_MEMORY_PROPERTY_HOST_COHERENT_BIT指定
	//2.当完成写入内存映射操作后，调用vkFlushMappedMemoryRanges函数，当读取映射内存之前，调用vkInvalidateMappedMemoryRanges函数
	VkResult CBuffer::flush(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		VkMappedMemoryRange MappedRange = {};
		MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		MappedRange.memory = m_DeviceMemory;
		MappedRange.offset = vOffset;
		MappedRange.size = vSize;
		return vkFlushMappedMemoryRanges(m_Device, 1, &MappedRange);
	}

	//当读取映射内存之前，调用vkInvalidateMappedMemoryRanges函数
	VkResult CBuffer::invalidate(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		VkMappedMemoryRange MappedRange = {};
		MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		MappedRange.memory = m_DeviceMemory;
		MappedRange.offset = vOffset;
		MappedRange.size = vSize;
		return vkInvalidateMappedMemoryRanges(m_Device, 1, &MappedRange);
	}

	//设置描述符
	void CBuffer::setupDescriptore(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0)
	{
		m_BufferDescriptor.offset = vOffset;
		m_BufferDescriptor.buffer = m_Buffer;
		m_BufferDescriptor.range = vSize;
	}
}


