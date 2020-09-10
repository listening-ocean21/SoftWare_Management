#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	class CCommandBuffer
	{
	public:
		CCommandBuffer();
		virtual ~CCommandBuffer() {};

		void begin();
		void end();
		void submit(VkSemaphore* vSignalSemaphore = nullptr);
		CCommandBuffer* createCommandBuffer(std::shared_ptr<CLogicalDevice> vDevice, VkCommandPool vCommandPool, VkCommandBufferLevel vLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VkCommandBuffer getCommandBuffer() const { return m_CommandBuffer; }
	private:
		VkCommandBuffer                   m_CommandBuffer = VK_NULL_HANDLE;
		VkCommandPool                     m_CommandPool = VK_NULL_HANDLE;
		std::shared_ptr<CLogicalDevice>     m_LogicalDevice = nullptr;
		std::vector<VkPipelineStageFlags> m_WaitPipelineStageFlags;

		VkFence                           m_Fence = VK_NULL_HANDLE;
		std::vector<VkSemaphore>          m_WaitSemaphores;
		bool                              m_IsBegun = false;  //标记是否已经开始记
	
		friend class CVertexBuffer;
	};
}
