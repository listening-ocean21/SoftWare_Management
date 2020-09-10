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

		void bindDraw(VkCommandBuffer cmdBuffer);

		CIndexBuffer* createIndexBuffer(std::shared_ptr<CLogicalDevice> vDevice, CCommandBuffer* cmdBuffer, std::vector<uint16_t> indices);
		CIndexBuffer* createIndexBuffer(std::shared_ptr<CLogicalDevice> vDevice, CCommandBuffer* cmdBuffer, std::vector<uint32_t> indices);
	private:
		VkDevice		m_Device = VK_NULL_HANDLE;
	    CBuffer*		m_IndexBuffer = nullptr;
		uint32_t        m_InstanceCount = 1;
		uint32_t        m_IndexCount = 1;
		VkIndexType		m_IndexType = VK_INDEX_TYPE_UINT16;  //��������Ҫָ�����д���65535��ʱ��
	};
}