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

//SDK通过请求VK_LAYER_LUNARG_standard_validaction层，
//来隐式的开启有所关于诊断layers，
//从而避免明确的指定所有的明确的诊断层。
const std::vector<const char*> ValidationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

//添加两个配置变量来指定要启用的layers以及是否开启它们
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

//该结构体应该传递给vkCreateDebugReportCallbackEXT函数创建VkDebugReportCallbackEXT对象
//不幸的是，因为这个功能是一个扩展功能，它不会被自动加载。
//所以必须使用vkGetInstanceProcAddr查找函数地址。我们将在后台创建代理函数
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

//清理VkDebugReportCallbackEXT对象
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
	//创建窗口
	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
	}

	//创建vulkan实例
	void initVulkan()
	{
		createInstance();
		//创建实例之后，需要在系统中找到一个支持功能的显卡，查找第一个图像卡作为适合物理设备
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

	//循环渲染
	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
		}
	}
	//退出
	void cleanUp()
	{
		destroyDebugReportCallbackEXT(m_Instance, m_CallBack, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
	//1.创建实例以及物理设备选择
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

		//需要一个扩展才能与不同平台的窗体系统进行交互
		//GLFW有一个方便的内置函数，返回它有关的扩展信息
		//我们可以传递给struct:
		auto Extensions = getRequiredExtensions();
		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());;
		CreateInfo.ppEnabledExtensionNames = Extensions.data();
		CreateInfo.enabledExtensionCount = 0;

		//修改VkInstanceCreateInfo结构体，填充当前上下文已经开启的validation layers名称集合。
		if (enableValidationLayers)
		{
			CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			CreateInfo.ppEnabledExtensionNames = ValidationLayers.data();
		}
		else {
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}
		//创建Instance
		//检查是否创建成功
		//TODO：需要调试
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
		if (Result != VK_SUCCESS) throw std::runtime_error("faild to createInstance!");

		//DebugInfo
		VkDebugReportCallbackCreateInfoEXT CreateDebugInfo = { };
		CreateDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		CreateDebugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		CreateDebugInfo.pfnCallback = debugCallBack;
	}

	//检查物理设备
	void pickPhysicalDevice()
	{
		VkPhysicalDevice PhysicakDevice = VK_NULL_HANDLE;
		//获取图像卡列表
		uint32_t DeviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr);
		if (DeviceCount == 0) throw std::runtime_error("failed to find GPUS with Vulkan support!");

		//分配数组存储所有的句柄
		std::vector<VkPhysicalDevice> Devices(DeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Devices.data());

		//检查它们是否适合我们要执行的操作
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

	//判断是否支持gPU函数
	bool isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices = findQueueFamilies(vDevice);
		return Indices.isComplete();
	}

	//检测设备中支持的队列蔟
	SQueueFamilyIndices findQueueFamilies(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices;

		uint32_t QueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

		//找到一个支持VK_QUEUE_GRAPHICS_BIT的队列簇
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

	//检查所有请求的layers是否可用
	bool checkValidationLayerSupport()
	{
		uint32_t LayerCount;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		std::vector<VkLayerProperties> AvailableLayers(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

		//检查所有的Layers是否存在与availabeLayers列表中
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

	//基于是否开启validation layers返回需要的扩展列表,类似于回调函数
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