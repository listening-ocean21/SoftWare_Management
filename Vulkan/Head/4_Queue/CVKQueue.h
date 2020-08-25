#pragma once
#include<vulkan\vulkan.h>
#include <memory>

class CLogicalDevice;
namespace vk_Demo
{
	class CVKQueue
	{
	public:

		CVKQueue(CLogicalDevice* vDevice, uint32_t vFamilyIndex);

		virtual ~CVKQueue();

		inline uint32_t getFamilyIndex() const { return m_FamilyIndex; }

		inline VkQueue getHandle() const { return m_Queue; }

	private:
		VkQueue           m_Queue;
		uint32_t          m_FamilyIndex;
		CLogicalDevice*   m_Device;
	};
}
