#include <vulkan\vulkan.h>
#include<vector>

#include "../../Head/2_Buffer/CBuffer.h"
#include "../../Head/1_Device/CLogicalDevice.h"
#include "../../Head/3_CommandBuffer/CCommandBuffer.h"
namespace vk_Demo
{
	class CIndexBuffer
	{
	public:
		CIndexBuffer() {};
		~CIndexBuffer();


		void bind(VkCommandBuffer cmdBuffer);

		static CIndexBuffer* createindexBuffer(std::shared_ptr<CLogicalDevice> vulkanDevice, CCommandBuffer* cmdBuffer, std::vector<uint16_t> indices);
	private:
		VkDevice		m_Device = VK_NULL_HANDLE;
	    CBuffer*		m_IndexBuffer = nullptr;
		uint32_t        m_InstanceCount = 1;
		uint32_t        m_IndexCount = 1;
		VkIndexType		m_IndexType = VK_INDEX_TYPE_UINT16;  //索引数需要指定，有大于65535的时候
	};
}