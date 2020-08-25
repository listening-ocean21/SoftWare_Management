#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>

const int WIDTH = 800;
const int HEIGHT = 600;

//SDKͨ������VK_LAYER_LUNARG_standard_validaction�㣬
//����ʽ�Ŀ��������������layers��
//�Ӷ�������ȷ��ָ�����е���ȷ����ϲ㡣
const std::vector<const char*> ValidationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

//����������ñ�����ָ��Ҫ���õ�layers�Լ��Ƿ�������
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct SQueueFamilyIndices
{
	int GraphicsFamily = -1;

	bool isComplete()
	{
		return GraphicsFamily >= 0;
	}
};

//�ýṹ��Ӧ�ô��ݸ�vkCreateDebugReportCallbackEXT��������VkDebugReportCallbackEXT����
//���ҵ��ǣ���Ϊ���������һ����չ���ܣ������ᱻ�Զ����ء�
//���Ա���ʹ��vkGetInstanceProcAddr���Һ�����ַ�����ǽ��ں�̨����������
VkResult ceateDebugReportCallBackEXT(VkInstance vInstance, 
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(vInstance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

}

//����VkDebugReportCallbackEXT����
void destroyDebugReportCallbackEXT(VkInstance vInstance,
	VkDebugReportCallbackEXT vCallback, 
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vInstance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(vInstance, vCallback, pAllocator);
	}
}

class CApplicationTest
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}
private:
	GLFWwindow * m_Window;
	VkInstance m_Instance;
	VkDebugReportCallbackEXT m_CallBack;
	//��������
	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
	}

	//����vulkanʵ��
	void initVulkan()
	{
		createInstance();
		//����ʵ��֮����Ҫ��ϵͳ���ҵ�һ��֧�ֹ��ܵ��Կ������ҵ�һ��ͼ����Ϊ�ʺ������豸
		pickPhysicalDevice();
		setupDebugCallBack();
	}

	//
	void populateDebugMessengerCreateInfo(VkDebugReportCallbackCreateInfoEXT& vDebugMessengerCreateInfo)
	{
		vDebugMessengerCreateInfo = {};
		vDebugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		vDebugMessengerCreateInfo.pfnCallback = debugCallBack;
	}
	//
	void setupDebugCallBack()
	{
		if (!enableValidationLayers) return;

		VkDebugReportCallbackCreateInfoEXT CreateDebugMessengerInfo;
		populateDebugMessengerCreateInfo(CreateDebugMessengerInfo);

		if (ceateDebugReportCallBackEXT(m_Instance, &CreateDebugMessengerInfo, nullptr, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	//ѭ����Ⱦ
	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
		}
	}
	//�˳�
	void cleanUp()
	{
		destroyDebugReportCallbackEXT(m_Instance, m_CallBack, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
	//1.����ʵ���Լ������豸ѡ��
	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo AppInfo = { };
		AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		AppInfo.pNext = nullptr;
		AppInfo.pApplicationName = "Hello World!";
		AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.pEngineName = "No Engine";
		AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo  CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &AppInfo;

		//��Ҫһ����չ�����벻ͬƽ̨�Ĵ���ϵͳ���н���
		//GLFW��һ����������ú������������йص���չ��Ϣ
		//���ǿ��Դ��ݸ�struct:
		auto Extensions = getRequiredExtensions();
		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());;
		CreateInfo.ppEnabledExtensionNames = Extensions.data();
		CreateInfo.enabledExtensionCount = 0;

		//�޸�VkInstanceCreateInfo�ṹ�壬��䵱ǰ�������Ѿ�������validation layers���Ƽ��ϡ�
		if (enableValidationLayers)
		{
			CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			CreateInfo.ppEnabledExtensionNames = ValidationLayers.data();
		}
		else {
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}
		//����Instance
		//����Ƿ񴴽��ɹ�
		//TODO����Ҫ����
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
		if (Result != VK_SUCCESS) throw std::runtime_error("faild to createInstance!");

		//DebugInfo
		VkDebugReportCallbackCreateInfoEXT CreateDebugInfo = { };
		CreateDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		CreateDebugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		CreateDebugInfo.pfnCallback = debugCallBack;
	}

	//��������豸
	void pickPhysicalDevice()
	{
		VkPhysicalDevice PhysicakDevice = VK_NULL_HANDLE;
		//��ȡͼ���б�
		uint32_t DeviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr);
		if (DeviceCount == 0) throw std::runtime_error("failed to find GPUS with Vulkan support!");

		//��������洢���еľ��
		std::vector<VkPhysicalDevice> Devices(DeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Devices.data());

		//��������Ƿ��ʺ�����Ҫִ�еĲ���
		for (const auto& device : Devices)
		{
			if (isDeviceSuitable(device))
			{
				PhysicakDevice = device;
				break;
			}
		}

		if (PhysicakDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	//�ж��Ƿ�֧��gPU����
	bool isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices = findQueueFamilies(vDevice);
		return Indices.isComplete();
	}

	//����豸��֧�ֵĶ�����
	SQueueFamilyIndices findQueueFamilies(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices;

		uint32_t QueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

		//�ҵ�һ��֧��VK_QUEUE_GRAPHICS_BIT�Ķ��д�
		int i = 0;
		for (const auto& QueueFamily : QueueFamilies)
		{
			if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				Indices.GraphicsFamily = i;
			}

			if (Indices.isComplete()) break;
			i++;
		}
		return Indices;
	}

	//������������layers�Ƿ����
	bool checkValidationLayerSupport()
	{
		uint32_t LayerCount;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		std::vector<VkLayerProperties> AvailableLayers(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

		//������е�Layers�Ƿ������availabeLayers�б���
		for (const char* layName : ValidationLayers)
		{
			bool LayerFound = false;
			for (const auto& layerProperties : AvailableLayers)
			{
				if (strcmp(layName, layerProperties.layerName) == 0)
				{
					LayerFound = true;
					break;
				}
			}
			if (!LayerFound)
			{
				return false;
			}
		}
		return true;
	}

	//�����Ƿ���validation layers������Ҫ����չ�б�,�����ڻص�����
	std::vector<const char*> getRequiredExtensions()
	{
		std::vector<const char*> Extensions;

		unsigned int GlfwExtensionCount = 0;
		const char** GlfwExtensions;
		GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

		for (unsigned int i = 0; i < GlfwExtensionCount; i++)
		{
			Extensions.push_back(GlfwExtensions[i]);
		}

		if (enableValidationLayers)
		{
			Extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return Extensions;
	}

	//callback
	static  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
		VkDebugReportFlagsEXT vfalgs,
		VkDebugReportObjectTypeEXT vObjType,
		uint64_t vObj,
		size_t vLocation,
		int32_t vCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData) {
		std::cerr << "validation layer:" << pMsg << std::endl;
		return VK_FALSE;
	}
};