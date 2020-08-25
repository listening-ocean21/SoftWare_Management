#include "../../Head/4_Queue/CVKQueue.h"
#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	//�������߼��豸�Զ���һͬ����,����Ҫ������ֱ�ӻ�ȡ
	CVKQueue::CVKQueue(CLogicalDevice* vDevice, uint32_t vFamilyIndex) : m_Queue(VK_NULL_HANDLE), m_FamilyIndex(vFamilyIndex), m_Device(vDevice)
	{
		vkGetDeviceQueue(m_Device->getInstanceHandle(), m_FamilyIndex, 0, &m_Queue);
	}

	//�豸�������豸�����ٵ�ʱ����ʽ����
	CVKQueue::~CVKQueue(){}
}
