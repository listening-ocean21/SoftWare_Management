#include "../../Head/3_CommandBuffer/CCommandBuffer.h"

namespace vk_Demo
{
	//CommandBuffer会在CommandPool销毁时自动销毁
	CCommandBuffer::~CCommandBuffer()
	{	
		if (m_CommandBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(m_LogicalDevice->getInstanceHandle(), m_CommandPool, 1, &m_CommandBuffer);
			m_CommandBuffer = VK_NULL_HANDLE;
		}
	}

	//创建CommandBuffer
	CCommandBuffer* CCommandBuffer::createCommandBuffer(std::shared_ptr<CLogicalDevice> vVulkanDevice, VkCommandPool vCommandPool, VkCommandBufferLevel vLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		VkDevice Device = vVulkanDevice->getInstanceHandle();

		CCommandBuffer* CmdBuffer = new CCommandBuffer();
		CmdBuffer->m_LogicalDevice = vVulkanDevice;
		CmdBuffer->m_CommandPool = vCommandPool;
		CmdBuffer->m_IsBegun = false;

		VkCommandBufferAllocateInfo CmdBufferAllocateInfo;
		CmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CmdBufferAllocateInfo.commandPool = vCommandPool;
		CmdBufferAllocateInfo.level = vLevel;
		CmdBufferAllocateInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(Device, &CmdBufferAllocateInfo, &(CmdBuffer->m_CommandBuffer)) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}

		VkFenceCreateInfo FenceCreateInfo;
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = 0;
		if (vkCreateFence(Device, &FenceCreateInfo, nullptr, &(CmdBuffer->m_Fence)) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create fence!");
		}

		return CmdBuffer;
	}
	

	//开始命令缓冲区的记录
	void CCommandBuffer::begin()
	{
		if (m_IsBegun)
		{
			return;
		}
		m_IsBegun = true;

		VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
		CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  //命令缓冲区将在执行一次后立即重新记录

		vkBeginCommandBuffer(m_CommandBuffer, &CmdBufferBeginInfo);
	}

    //结束命令缓冲区的记录
	void CCommandBuffer::end()
	{
		if (!m_IsBegun)
		{
			return;
		}
		m_IsBegun = false;

		vkEndCommandBuffer(m_CommandBuffer);
	}

	//提交命令缓冲区到队列
	void CCommandBuffer::submit(VkSemaphore* vSignalSemaphore = nullptr)
	{
		end();

		VkSubmitInfo SubmitInfo{};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &m_CommandBuffer;
		SubmitInfo.signalSemaphoreCount = vSignalSemaphore ? 1 : 0;
		SubmitInfo.pSignalSemaphores = vSignalSemaphore;  //ommandBuffers 中指定的命令完成执行，将被发送信号的信号量。

		if (m_WaitPipelineStageFlags.size() > 0)
		{
			SubmitInfo.waitSemaphoreCount = m_WaitSemaphores.size();
			SubmitInfo.pWaitSemaphores = m_WaitSemaphores.data(); //ommandBuffer在提交之前需要等待的信号量
			SubmitInfo.pWaitDstStageMask = m_WaitPipelineStageFlags.data();
		}

		//重置Fence,防止没提交之前就有signaled
		vkResetFences(m_LogicalDevice->getInstanceHandle(), 1, &m_Fence);

		vkQueueSubmit(m_LogicalDevice->getGraphicsQueue()->GetHandle(), 1, &SubmitInfo, m_Fence);
		
		vkWaitForFences(m_LogicalDevice->getInstanceHandle(), 1, &m_Fence, true, UINT64_MAX); //完成提交命令时，通知Fence
	}
}
