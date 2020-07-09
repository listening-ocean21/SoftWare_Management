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
	"VK_LAYER_KHRONOS_validation"
};

//添加两个配置变量来指定要启用的layers以及是否开启它们
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

//填写完结构体VkDebugUtilsMessengerCreateInfoEXT 信息后，将它作为一个参数调用vkCreateDebugUtilsMessengerEXT函数来创建VkDebugUtilsMessengerEXT对象。
//但由于vkCreateDebugUtilsMessengerEXT函数是一个扩展函数，不会被Vulkan库自动加载，需要使用vkGetInstanceProcAddr函数来加载
//FUNCTION:一个代理函数，主要是为了加载vkCreateDebugUtilsMessengerEXT函数
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


//清理vkDestroyDebugUtilsMessengerEXT对象
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

	//创建窗口
	void __initWindow()
	{
		glfwInit();
		//由于GLFW库最初是为了OpenGL设计，需要显式设置GLFW阻止它自动创建OpenGL上下文
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//禁止窗口大小改变 
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
	}

	//创建vulkan实例
	void __initVulkan()
	{
		__createInstance();
		//创建实例之后，需要在系统中找到一个支持功能的显卡，查找第一个图像卡作为适合物理设备
		__pickPhysicalDevice();
		__createLogicalDevice();
		__setupDebugCallBack();
	}

	//循环渲染
	void __mainLoop()
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
		}
	}
	//退出
	void __cleanUp()
	{
		//使用Vulkan API创建的对象也需要被清除，且应该在Vulkan实例清除之前被清除。
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

		//1.1应用程序信息
		VkApplicationInfo AppInfo = { };
		AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		//AppInfo.pNext = nullptr;
		AppInfo.pApplicationName = "Hello World!";
		AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.pEngineName = "No Engine";
		AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		AppInfo.apiVersion = VK_API_VERSION_1_0;

		//1.2Vulkan实例信息：设置Vulkan的驱动程序需要使用的全局扩展和校验层
		VkInstanceCreateInfo  CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &AppInfo;
		//需要一个扩展才能与不同平台的窗体系统进行交互
		//GLFW有一个方便的内置函数，返回它有关的扩展信息
		//传递给struct:
		auto Extensions = __getRequiredExtensions();
		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());;
		CreateInfo.ppEnabledExtensionNames = Extensions.data();
		//指定全局校验层
		CreateInfo.enabledExtensionCount = 0;

		//DebugInfo
		VkDebugUtilsMessengerCreateInfoEXT  CreateDebugInfo = { };
		
		//修改VkInstanceCreateInfo结构体，填充当前上下文已经开启的validation layers名称集合。
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

		//1.3创建Instance
		VkResult InstanceResult = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
		//检查是否创建成功
		if (InstanceResult != VK_SUCCESS) throw std::runtime_error("faild to create Instance!");

	}

	//选择支持我们需要的特性的设备
	void __pickPhysicalDevice()
	{
		//1.使用VkPhysicalDevice对象储存显卡信息，这个对象可以在Instance对象清除时，自动清除自己，所有不用人工清除
		VkPhysicalDevice PhysicakDevice = VK_NULL_HANDLE;
		//2.获取图像卡列表
		uint32_t DeviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr);
		if (DeviceCount == 0) throw std::runtime_error("failed to find GPUS with Vulkan support!");

		//3.获取完设备数量后，就可以分配数组来存储VkPhysicalDevice对象
		std::vector<VkPhysicalDevice> Devices(DeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Devices.data());

		//4.检查它们是否适合我们要执行的操作
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

	//创建逻辑设备：
	//FUNC:主要与物理设备进行交互
	void __createLogicalDevice()
	{
		//创建队列
		EQueueFamilyIndices  Indices = __findQueueFamilies(m_PhysicalDevice);

		//描述队列蔟中预定申请的队列个数，仅关心具备图形能力的队列
		VkDeviceQueueCreateInfo QueueCreateInfo = {};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = Indices.GraphicsFamily;
		QueueCreateInfo.queueCount = 1;

		//分配队列优先级
		float QueuePriority = 1.0f;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;

		//指定使用的设备特性
		VkPhysicalDeviceFeatures DeviceFeatures = {};

		//创建逻辑设备
		VkDeviceCreateInfo DeviceCreateInfo = {};
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//添加指向队列创建信息的结构体和设备功能结构体:
		DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
		DeviceCreateInfo.queueCreateInfoCount = 1;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		//为设备开启validation layers
		DeviceCreateInfo.enabledExtensionCount = 0;
		if (enableValidationLayers)
		{
			DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
			DeviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
		}
		else {
			DeviceCreateInfo.enabledLayerCount = 0;
		}

		//调用vkCreateDevice函数来创建实例化逻辑设备
		VkResult Result = vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, NULL, &m_Device);
		if (Result != VK_SUCCESS) throw std::runtime_error("failed to create logical device!");

		//获取指定队列族的队列句柄
		vkGetDeviceQueue(m_Device, Indices.GraphicsFamily, 0, &m_GraphicsQueue);
	}
	//判断是否支持gPU函数
	bool __isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		EQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);
		return Indices.isComplete();
	}

	//检测设备中支持的队列蔟
	EQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vDevice)
	{
		EQueueFamilyIndices  Indices;

		//设备队列族的个数
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

	//检查所有请求的校验层layers是否可用
	bool __checkValidationLayerSupport()
	{
		uint32_t LayerCount;
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

		std::vector<VkLayerProperties> AvailableLayers(LayerCount);
		vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

		//检查是否所有validationLayers列表中的校验层都可以在availableLayers列表中找到：
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

	//仅仅启用校验层并没有任何用处，我们不能得到任何有用的调试信息。
	//为了获得调试信息，我们需要使用VK_EXT_debug_utils扩展，设置回调函数来接受调试信息。
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

	//回调函数：返回Bool值，表示引发校验层处理的Vulkan API调用是否中断
	//返回True:对应Vulkan API调用就会返回VK_ERROR_VALIDATION_FAILED_EXT错误代码。通常，只在测试校验层本身时会返回true
	//其余情况下，回调函数应该返回VK_FALSE
	static  VKAPI_ATTR VkBool32 VKAPI_CALL __debugCallBack(
		VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity,   //指定消息的级别
		VkDebugUtilsMessageTypeFlagsEXT vMessageType,              //错误行为
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, //指向VkDebugUtilsMessengerCallbackDataEXT结构体指针，结构成员有：pMessage, pObject, objectCount
		void* pUserData)                                           //指向回调函数时，传递数据的指针 
	{
		std::cerr << "validation layer:" << pCallbackData -> pMessage << std::endl;
		return VK_FALSE;
	}

	//调用回调函数
	void __setupDebugCallBack()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT  CreateDebugMessengerInfo;

		if (createDebugUtilsMessengerEXT(m_Instance, &CreateDebugMessengerInfo, nullptr, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}


};
