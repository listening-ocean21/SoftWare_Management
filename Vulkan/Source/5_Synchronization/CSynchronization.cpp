#include "../../Head/5_Synchronization/CSynchronization.h"

namespace vk_Demo
{
	//Fence
	CFence::CFence(CLogicalDevice* vDevcie, bool vSignaled):m_Fence(VK_NULL_HANDLE), m_State(State::UNSIGNALED)
	{
		VkFenceCreateInfo CreateInfo;
		CreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		CreateInfo.flags = vSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		vkCreateFence(vDevcie->getInstanceHandle(), &CreateInfo, nullptr, &m_Fence);
	}

	//FenceManaget
	CFenceManager::~CFenceManager()
	{
		if (m_UsedFences.size() > 0) {
			throw std::runtime_error("No all fences are done!");
		}

		for (uint32_t i = 0; i < m_FreeFences.size(); ++i) {
			__destoryFence(m_FreeFences[i]);
		}
	}

	CFence* CFenceManager::createFence(bool vSignaled)
	{
		if (m_FreeFences.size() > 0)
		{
			CFence* Fence = m_FreeFences.back();
			m_FreeFences.pop_back();
			m_UsedFences.push_back(Fence);
			if (vSignaled) {
				//访问私有变量
				Fence->setFenceState(CFence::State::UNSIGNALED);
			}
			return Fence;
		}

		CFence* NewFence = new CFence(m_Device,vSignaled);
		m_UsedFences.push_back(NewFence);
		return NewFence;

	}
	bool CFenceManager::waitForFence(CFence* vFence, uint64_t VTimeInNanoseconds)
	{
		VkResult Result = vkWaitForFences(m_Device->getInstanceHandle(), 1, &(vFence->m_Fence), true, VTimeInNanoseconds);
		switch (Result)
		{
			case VK_SUCCESS:
				vFence->setFenceState(CFence::State::SIGNALED);
				return true;
			case VK_TIMEOUT:
				return false;
			default:
				throw std::runtime_error("Unkow error!");
				return false;
		}
	}
	void CFenceManager::resetFence(CFence* vFence)
	{
		if (vFence->m_State != CFence::State::UNSIGNALED)
		{
			vkResetFences(m_Device->getInstanceHandle(), 1, &vFence->m_Fence);
			vFence->setFenceState(CFence::State::UNSIGNALED);
		}
	}
	void CFenceManager::releaseFence(CFence* vFence)
	{

		resetFence(vFence);
		for (uint32_t i = 0; i < m_UsedFences.size(); ++i) {
			if (m_UsedFences[i] == vFence)
			{
				m_UsedFences.erase(m_UsedFences.begin() + i);
				break;
			}
		}
		m_FreeFences.push_back(vFence);
		vFence = nullptr;
	}
	void CFenceManager::waitAndReleaseFence(CFence* cFence, uint64_t vTimeInNanoseconds)
	{
		if (!cFence->isFenceSignaled()) {
			waitForFence(cFence, vTimeInNanoseconds);
		}
		releaseFence(cFence);
	}

	void CFenceManager::__destoryFence(CFence* vFence)
	{
		vkDestroyFence(m_Device->getInstanceHandle(), vFence->m_Fence, nullptr);
		vFence->m_Fence = VK_NULL_HANDLE;
		delete vFence;
	}

	bool CFenceManager::checkFenceState(CFence* vFence)
	{
		VkResult result = vkGetFenceStatus(m_Device->getInstanceHandle(), vFence->m_Fence);
		switch (result)
		{
		case VK_SUCCESS:
			vFence->m_State = CFence::State::SIGNALED;
			break;
		case VK_NOT_READY:
			break;
		default:
			break;
		}
		return false;
	}
	// Semaphore
	CSemaphore::CSemaphore(CLogicalDevice* vDevice) : m_Semaphore(VK_NULL_HANDLE), m_Device(vDevice)
	{
		VkSemaphoreCreateInfo CreateInfo;
		CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(vDevice->getInstanceHandle(), &CreateInfo, nullptr, &m_Semaphore);
	}

	CSemaphore::~CSemaphore()
	{
		if (m_Semaphore == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed destory VkSemaphore.");
		}
		vkDestroySemaphore(m_Device->getInstanceHandle(), m_Semaphore, nullptr);
	}
}