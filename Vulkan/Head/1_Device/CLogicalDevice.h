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
		VkDevice                                m_Device;   //逻辑设备
		VkPhysicalDevice                        m_PhysicalDevice;
		VkPhysicalDeviceProperties              m_PhysicalDeviceProperties; //查询设备name,type, version等属性
		VkPhysicalDeviceFeatures                m_PhysicalDeviceFeatures;   //查询对纹理压缩，64位浮点数和多视图渲染(VR非常有用)等可选功能的支持:
		std::vector<VkQueueFamilyProperties>    m_QueueFamilyProps;         //队列蔟属性

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