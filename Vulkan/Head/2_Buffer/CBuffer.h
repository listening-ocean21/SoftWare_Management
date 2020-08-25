#include <vulkan\vulkan.h>
#include <memory>

#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	class CBuffer
	{
	public:
		virtual ~CBuffer();
		CBuffer* createBuffer(std::shared_ptr<CLogicalDevice> vDevice, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vMemoryPropertyFlags, VkDeviceSize vSize, void* vData);

		VkResult map(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);  //�������������
		void unMap();
		VkResult bind(VkDeviceSize vOffset = 0);
		void copyToBuffer(void* vData, VkDeviceSize vSize);
		VkResult flush(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);
		VkResult invalidate(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);
		void setupDescriptore(VkDeviceSize vSize = VK_WHOLE_SIZE, VkDeviceSize vOffset = 0);
		VkBuffer getBuffer()  { return m_Buffer; }
	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo m_BufferDescriptor;
		VkBufferUsageFlags m_BufferUsageFlags;   
		VkMemoryPropertyFlags m_MemoryPropertyFlags;

		VkDeviceSize m_BufferSize = 0;
		VkDeviceSize m_Alignment = 0; //����Ҫ��

		void  *m_pMapped = nullptr;   //ָ�������ڴ��������豸�ڴ�ӳ���ָ��

		//���ι��캯����ͨ��createBuffer����Buffer
		CBuffer() {};
		friend class CVertexBuffer;
	};
}
