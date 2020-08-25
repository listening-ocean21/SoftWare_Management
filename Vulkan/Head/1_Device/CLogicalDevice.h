#include <vulkan\vulkan.h>
#include <vector>
#include <map>

#include "../../Head/4_Queue/CVKQueue.h"

namespace vk_Demo
{
	class CLogicalDevice
	{
	public:
		CLogicalDevice(VkPhysicalDevice vPhysicalDevice);
		virtual ~CLogicalDevice();
		void createDevice();
		inline std::shared_ptr<CVKQueue> getGraphicsQueue(){ return m_GraphicsQueue; }
		inline std::shared_ptr<CVKQueue> getComputeQueue(){	return m_ComputeQueue;}
		inline std::shared_ptr<CVKQueue> getTransferQueue(){	return m_TransferQueue;}
		inline std::shared_ptr<CVKQueue> getPresentQueue(){return m_PresentQueue;}
		inline const VkPhysicalDeviceProperties& getDeviceProperties() const{return m_PhysicalDeviceProperties;}
		inline const VkPhysicalDeviceLimits& getLimits() const{	return m_PhysicalDeviceProperties.limits;}
		inline const VkPhysicalDeviceFeatures& getPhysicalFeatures() const{	return m_PhysicalDeviceFeatures;}
		inline const VkFormatProperties& getFormatProperties() const { return m_FormatProperties; }
		inline VkDevice getInstanceHandle() const{	return m_Device;}
		inline VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }


	private:
		VkDevice                                m_Device;   //�߼��豸
		VkPhysicalDevice                        m_PhysicalDevice;
		VkPhysicalDeviceProperties              m_PhysicalDeviceProperties; //��ѯ�豸name,type, version������
		VkPhysicalDeviceFeatures                m_PhysicalDeviceFeatures;   //��ѯ������ѹ����64λ�������Ͷ���ͼ��Ⱦ(VR�ǳ�����)�ȿ�ѡ���ܵ�֧��:
		std::vector<VkQueueFamilyProperties>    m_QueueFamilyProps;         //����������

		VkFormatProperties                      m_FormatProperties;
		std::map<VkFormat, VkFormatProperties>  m_ExtensionFormatProperties;
		VkComponentMapping                      m_PixelFormatComponentMapping;

		std::shared_ptr<CVKQueue>            m_GraphicsQueue;
		std::shared_ptr<CVKQueue>            m_ComputeQueue;
		std::shared_ptr<CVKQueue>            m_TransferQueue;
		std::shared_ptr<CVKQueue>            m_PresentQueue;

		
		std::vector<const char*>			 m_DeviceExtensions;
		std::vector<const char*>			 m_DeviceValidationLayers;

		void __getDeviceExtensionsAndLayers(std::vector<const char*>& vOutDeviceExtensions, std::vector<const char*>& vOutDeviceLayers, bool& vBOutDebugMarkers);
	};
}