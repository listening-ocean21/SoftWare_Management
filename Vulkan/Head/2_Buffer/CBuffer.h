#include <memory>

#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	class CBuffer
	{
	public:
		virtual ~CBuffer();
		static CBuffer* createBuffer(std::shared_ptr<CLogicalDevice> vDevice, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vMemoryPropertyFlags, VkDeviceSize vSize, void* vData = nullptr);

		void unMap();
		void copyToBuffer(void* vData, VkDeviceSize vSize);
		void setupDescriptore(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);

		VkResult bind(VkDeviceSize vOffset = 0);
		VkBuffer getHandle()  const { return m_Buffer; }
		VkDescriptorBufferInfo getDescriptorBufferInfo() const { return m_BufferDescriptor;}
		VkResult map(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);  //填充整个缓冲区
		VkResult flush(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);
		VkResult invalidate(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo m_BufferDescriptor;
		VkBufferUsageFlags m_BufferUsageFlags;   
		VkMemoryPropertyFlags m_MemoryPropertyFlags;

		VkDeviceSize m_BufferSize = 0;
		VkDeviceSize m_Alignment = 0; //对齐要求

		void  *m_pMapped = nullptr;   //指向主机内存与物理设备内存映射的指针

		//屏蔽构造函数，通过createBuffer创建Buffer
		CBuffer() {};
		friend class CVertexBuffer;
	};
}
