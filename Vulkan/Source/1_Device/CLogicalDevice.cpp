#include <GLFW/glfw3.h>
#include "../../Head/1_Device/CLogicalDevice.h"

namespace vk_Demo
{
	CLogicalDevice::CLogicalDevice(VkPhysicalDevice vPhysicalDevice) : m_Device(VK_NULL_HANDLE)
		, m_PhysicalDevice(vPhysicalDevice)
		, m_GraphicsQueue(nullptr)
		, m_ComputeQueue(nullptr)
		, m_TransferQueue(nullptr)
		, m_PresentQueue(nullptr)
	{

	}

	CLogicalDevice:: ~CLogicalDevice()
	{
		if (m_Device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_Device, nullptr);
			m_Device = VK_NULL_HANDLE;
		}
	}

	void CLogicalDevice::createDevice()
	{
		bool DebugMarkersFound = false;
		//获取Extension和ValidationLayers
		__getDeviceExtensionsAndLayers(m_DeviceExtensions, m_DeviceValidationLayers, DebugMarkersFound);

		if (m_DeviceExtensions.size() > 0)
		{
			return;
		}

		VkDeviceCreateInfo DeviceInfo;
		DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceInfo.enabledExtensionCount = uint32_t(m_DeviceExtensions.size());
		DeviceInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		DeviceInfo.enabledLayerCount = uint32_t(m_DeviceValidationLayers.size());
		DeviceInfo.ppEnabledLayerNames = m_DeviceValidationLayers.data();

		DeviceInfo.pEnabledFeatures = &m_PhysicalDeviceFeatures;

		//寻找设备支持的队列簇
		std::vector<VkDeviceQueueCreateInfo> QueueFamilyInfos;

		//优先级
		uint32_t Priorities = 0;
		uint32_t GraphicsQueueFamilyIndex = -1;
		uint32_t ComputeQueueFamilyIndex = -1;
		uint32_t TransferQueueFamilyIndex = -1;

		for (uint32_t familyIndex = 0; familyIndex < m_QueueFamilyProps.size(); ++familyIndex)
		{
			const VkQueueFamilyProperties& CurrProps = m_QueueFamilyProps[familyIndex];
			bool isValidQueue = false;

			if ((CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
			{
				if (GraphicsQueueFamilyIndex == -1) {
					GraphicsQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
			}

			if ((CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
			{
				if (ComputeQueueFamilyIndex == -1)
				{
					ComputeQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
			}

			if ((CurrProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
			{
				if (TransferQueueFamilyIndex == -1)
				{
					TransferQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
			}

			if (!isValidQueue)
			{
				continue;
			}

			//存储
			VkDeviceQueueCreateInfo CurrQueue;
			CurrQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			CurrQueue.queueFamilyIndex = familyIndex;
			CurrQueue.queueCount = CurrProps.queueCount;
			Priorities += CurrProps.queueCount;
			QueueFamilyInfos.push_back(CurrQueue);
		}

		//统计各个队列簇的优先级
		std::vector<float> QueuePriorities(Priorities);
		float* CurrentPriority = QueuePriorities.data();
		for (uint32_t index = 0; index < QueueFamilyInfos.size(); ++index)
		{
			VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[index];
			CurrQueue.pQueuePriorities = CurrentPriority;
			const VkQueueFamilyProperties& CurrProps = m_QueueFamilyProps[CurrQueue.queueFamilyIndex];
			for (uint32_t queueIndex = 0; queueIndex < (uint32_t)CurrQueue.queueCount; ++queueIndex)
			{
				*CurrentPriority++ = 1.0f;
			}
		}

		DeviceInfo.queueCreateInfoCount = uint32_t(QueueFamilyInfos.size());
		DeviceInfo.pQueueCreateInfos = QueueFamilyInfos.data();

		VkResult result = vkCreateDevice(m_PhysicalDevice, &DeviceInfo, nullptr, &m_Device);
		if (result == VK_ERROR_INITIALIZATION_FAILED)
		{
			throw std::runtime_error("Cannot create a Vulkan device. Try updating your video driver to a more recent version.\n");
			return;
		}

		m_GraphicsQueue = std::make_shared<CVKQueue>(this, GraphicsQueueFamilyIndex);

		if (ComputeQueueFamilyIndex == -1) {
			ComputeQueueFamilyIndex = GraphicsQueueFamilyIndex;
		}
		m_ComputeQueue = std::make_shared<CVKQueue>(this, ComputeQueueFamilyIndex);

		if (TransferQueueFamilyIndex == -1) {
			TransferQueueFamilyIndex = ComputeQueueFamilyIndex;
		}
		m_TransferQueue = std::make_shared<CVKQueue>(this, TransferQueueFamilyIndex);
	}

	//TODO
	void CLogicalDevice::__getDeviceExtensionsAndLayers(std::vector<const char*>& vOutDeviceExtensions, std::vector<const char*>& vOutDeviceLayers, bool& vBOutDebugMarkers)
	{
		uint32_t GlfwExtensionCount = 0;
		const char** GlfwExtensions;
		GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

		std::vector<const char*> extensions(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);


		uint32_t LayerCount;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, availableLayers.data());

		for (const char* layerName : vOutDeviceLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				throw std::runtime_error("Can not findf layer!");
			}
		}
	}
}
