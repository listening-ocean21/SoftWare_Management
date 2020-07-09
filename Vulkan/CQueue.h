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
	"VK_LAYER_KHRONOS_validation"
};

//����������ñ�����ָ��Ҫ���õ�layers�Լ��Ƿ�������
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct EQueueFamilyIndices
{
	int GraphicsFamily = -1;

	bool isComplete()
	{
		return GraphicsFamily >= 0;
	}
};

//��д��ṹ��VkDebugUtilsMessengerCreateInfoEXT ��Ϣ�󣬽�����Ϊһ����������vkCreateDebugUtilsMessengerEXT����������VkDebugUtilsMessengerEXT����
//������vkCreateDebugUtilsMessengerEXT������һ����չ���������ᱻVulkan���Զ����أ���Ҫʹ��vkGetInstanceProcAddr����������
//FUNCTION:һ������������Ҫ��Ϊ�˼���vkCreateDebugUtilsMessengerEXT����
VkResult createDebugUtilsMessengerEXT(VkInstance vInstance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance,"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(vInstance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


//����vkDestroyDebugUtilsMessengerEXT����
void destroyDebugUtilsMessengerEXT(VkInstance vInstance,
	VkDebugUtilsMessengerEXT vCallback,
	const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(vInstance,
			"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(vInstance, vCallback, pAllocator);
	}
}


class CQueue
{
public:
	void run()
	{
		__initWindow();
		__initVulkan();
		__mainLoop();
		__cleanUp();
	}
private:
	GLFWwindow * m_pWindow;
	VkInstance m_Instance;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device;

	VkQueue m_GraphicsQueue;

	VkDebugUtilsMessengerEXT m_CallBack;

	//��������
	void __initWindow()
	{
		glfwInit();
		//����GLFW�������Ϊ��OpenGL��ƣ���Ҫ��ʽ����GLFW��ֹ���Զ�����OpenGL������
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//��ֹ���ڴ�С�ı� 
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
	}

	//����vulkanʵ��
	void __initVulkan()
	{
		__createInstance();
		//����ʵ��֮����Ҫ��ϵͳ���ҵ�һ��֧�ֹ��ܵ��Կ������ҵ�һ��ͼ����Ϊ�ʺ������豸
		__pickPhysicalDevice();
		__createLogicalDevice();
		__setupDebugCallBack();
	}

	//ѭ����Ⱦ
	void __mainLoop()
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
		}
	}
	//�˳�
	void __cleanUp()
	{
		//ʹ��Vulkan API�����Ķ���Ҳ��Ҫ���������Ӧ����Vulkanʵ�����֮ǰ�������
		if (enableValidationLayers)
		{
			destroyDebugUtilsMessengerEXT(m_Instance, m_CallBack, nullptr);
		}
		vkDestroyInstance(m_Instance, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}
	void __createInstance()
	{
		if (enableValidationLayers && !__checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		//1.1Ӧ�ó�����Ϣ
		VkApplicationInfo AppInfo = { };
		AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		//AppInfo.pNext = nullptr;
		AppInfo.pApplicationName = "Hello World!";
		AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.pEngineName = "No Engine";
		AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.apiVersion = VK_API_VERSION_1_0;

		//1.2Vulkanʵ����Ϣ������Vulkan������������Ҫʹ�õ�ȫ����չ��У���
		VkInstanceCreateInfo  CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &AppInfo;
		//��Ҫһ����չ�����벻ͬƽ̨�Ĵ���ϵͳ���н���
		//GLFW��һ����������ú������������йص���չ��Ϣ
		//���ݸ�struct:
		auto Extensions = __getRequiredExtensions();
		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());;
		CreateInfo.ppEnabledExtensionNames = Extensions.data();
		//ָ��ȫ��У���
		CreateInfo.enabledExtensionCount = 0;

		//DebugInfo
		VkDebugUtilsMessengerCreateInfoEXT  CreateDebugInfo = { };
		
		//�޸�VkInstanceCreateInfo�ṹ�壬��䵱ǰ�������Ѿ�������validation layers���Ƽ��ϡ�
		if (enableValidationLayers)
		{
			CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
			CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&CreateDebugInfo;
		}
		else {
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}

		//1.3����Instance
		VkResult InstanceResult = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
		//����Ƿ񴴽��ɹ�
		if (InstanceResult != VK_SUCCESS) throw std::runtime_error("faild to create Instance!");

	}

	//ѡ��֧��������Ҫ�����Ե��豸
	void __pickPhysicalDevice()
	{
		//1.ʹ��VkPhysicalDevice���󴢴��Կ���Ϣ��������������Instance�������ʱ���Զ�����Լ������в����˹����
		VkPhysicalDevice PhysicakDevice = VK_NULL_HANDLE;
		//2.��ȡͼ���б�
		uint32_t DeviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr);
		if (DeviceCount == 0) throw std::runtime_error("failed to find GPUS with Vulkan support!");

		//3.��ȡ���豸�����󣬾Ϳ��Է����������洢VkPhysicalDevice����
		std::vector<VkPhysicalDevice> Devices(DeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Devices.data());

		//4.��������Ƿ��ʺ�����Ҫִ�еĲ���
		for (const auto& device : Devices)
		{
			if (__isDeviceSuitable(device))
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

	//�����߼��豸��
	//FUNC:��Ҫ�������豸���н���
	void __createLogicalDevice()
	{
		//��������
		EQueueFamilyIndices  Indices = __findQueueFamilies(m_PhysicalDevice);

		//������������Ԥ������Ķ��и����������ľ߱�ͼ�������Ķ���
		VkDeviceQueueCreateInfo QueueCreateInfo = {};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = Indices.GraphicsFamily;
		QueueCreateInfo.queueCount = 1;

		//����������ȼ�
		float QueuePriority = 1.0f;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;

		//ָ��ʹ�õ��豸����
		VkPhysicalDeviceFeatures DeviceFeatures = {};

		//�����߼��豸
		VkDeviceCreateInfo DeviceCreateInfo = {};
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//���ָ����д�����Ϣ�Ľṹ����豸���ܽṹ��:
		DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
		DeviceCreateInfo.queueCreateInfoCount = 1;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		//Ϊ�豸����validation layers
		DeviceCreateInfo.enabledExtensionCount = 0;
		if (enableValidationLayers)
		{
			DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			DeviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
		}
		else {
			DeviceCreateInfo.enabledLayerCount = 0;
		}

		//����vkCreateDevice����������ʵ�����߼��豸
		VkResult Result = vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, NULL, &m_Device);
		if (Result != VK_SUCCESS) throw std::runtime_error("failed to create logical device!");

		//��ȡָ��������Ķ��о��
		vkGetDeviceQueue(m_Device, Indices.GraphicsFamily, 0, &m_GraphicsQueue);
	}
	//�ж��Ƿ�֧��gPU����
	bool __isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		EQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);
		return Indices.isComplete();
	}

	//����豸��֧�ֵĶ�����
	EQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vDevice)
	{
		EQueueFamilyIndices  Indices;

		//�豸������ĸ���
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

	//������������У���layers�Ƿ����
	bool __checkValidationLayerSupport()
	{
		uint32_t LayerCount;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		std::vector<VkLayerProperties> AvailableLayers(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

		//����Ƿ�����validationLayers�б��е�У��㶼������availableLayers�б����ҵ���
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

	//��������У��㲢û���κ��ô������ǲ��ܵõ��κ����õĵ�����Ϣ��
	//Ϊ�˻�õ�����Ϣ��������Ҫʹ��VK_EXT_debug_utils��չ�����ûص����������ܵ�����Ϣ��
	std::vector<const char*> __getRequiredExtensions()
	{
		unsigned int GlfwExtensionCount = 0;
		const char** GlfwExtensions;
		GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

		std::vector<const char*> Extensions(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);

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

	//�ص�����������Boolֵ����ʾ����У��㴦���Vulkan API�����Ƿ��ж�
	//����True:��ӦVulkan API���þͻ᷵��VK_ERROR_VALIDATION_FAILED_EXT������롣ͨ����ֻ�ڲ���У��㱾��ʱ�᷵��true
	//��������£��ص�����Ӧ�÷���VK_FALSE
	static  VKAPI_ATTR VkBool32 VKAPI_CALL __debugCallBack(
		VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity,   //ָ����Ϣ�ļ���
		VkDebugUtilsMessageTypeFlagsEXT vMessageType,              //������Ϊ
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, //ָ��VkDebugUtilsMessengerCallbackDataEXT�ṹ��ָ�룬�ṹ��Ա�У�pMessage, pObject, objectCount
		void* pUserData)                                           //ָ��ص�����ʱ���������ݵ�ָ�� 
	{
		std::cerr << "validation layer:" << pCallbackData -> pMessage << std::endl;
		return VK_FALSE;
	}

	//���ûص�����
	void __setupDebugCallBack()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT  CreateDebugMessengerInfo;

		if (createDebugUtilsMessengerEXT(m_Instance, &CreateDebugMessengerInfo, nullptr, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}


};
