#include "../../Head/4_Queue/CVKQueue.h"
#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	//队列与逻辑设备自动的一同创建,不需要创建，直接获取
	CVKQueue::CVKQueue(CLogicalDevice* vDevice, uint32_t vFamilyIndex) : m_Queue(VK_NULL_HANDLE), m_FamilyIndex(vFamilyIndex), m_Device(vDevice)
	{
		vkGetDeviceQueue(m_Device->getInstanceHandle(), m_FamilyIndex, 0, &m_Queue);
	}

	//设备队列在设备被销毁的时候隐式清理
	CVKQueue::~CVKQueue(){}
}
