#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <set>
#include <algorithm>

const int WIDTH = 800;
const int HEIGHT = 600;

//SDK通过请求VK_LAYER_LUNARG_standard_validaction层，
//来隐式的开启有所关于诊断layers，
//从而避免明确的指定所有的明确的诊断层。
const std::vector<const char*> ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

//定义所需的设备扩展列表
const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//添加两个配置变量来指定要启用的layers以及是否开启它们
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//具体的Vulkan实现可能对窗口系统进行了支持，但这并不意味着所有平台的Vulkan实现都支持同样的特性
//需要扩展isDeviceSuitable函数来确保设备可以在我们创建的表面上显示图像
struct EQueueFamilyIndices
{	//支持绘制指令的队列簇和支持表现的队列簇
	int GraphicsFamily = -1;
	int PresentFamily = -1;

	bool isComplete()
	{
		return (GraphicsFamily >= 0 && PresentFamily >= 0);
	}
};

//检查交换链的细节：1.surface特性  2.surface格式 3.可用的呈现模式
struct ESwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR Capablities;
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> PresentModes;
};
//填写完结构体VkDebugUtilsMessengerCreateInfoEXT 信息后，将它作为一个参数调用vkCreateDebugUtilsMessengerEXT函数来创建VkDebugUtilsMessengerEXT对象。
//但由于vkCreateDebugUtilsMessengerEXT函数是一个扩展函数，不会被Vulkan库自动加载，需要使用vkGetInstanceProcAddr函数来加载
//FUNCTION:一个代理函数，主要是为了加载vkCreateDebugUtilsMessengerEXT函数
VkResult createDebugUtilsMessengerEXT(VkInstance vInstance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugReportCallbackEXT");
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


class CSwapChain
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
	VkQueue m_PresentQueue;

	VkDebugUtilsMessengerEXT m_CallBack;

	VkSurfaceKHR m_WindowSurface;

	//存储交换链的
	VkSwapchainKHR m_SwapChain;
	//获取交换链图像的图像句柄
   //交换链图像由交换链自己负责创建，并在交换链清除时自动被清除，不需要我们自己进行创建和清除操作
	std::vector<VkImage> m_SwapChainImages;
	VkFormat   m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;

	//存储图像视图的句柄集
	std::vector<VkImageView> m_SwapChainImageViews;


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
		__setupDebugCallBack();
		__createSurface();
		//创建实例之后，需要在系统中找到一个支持功能的显卡，查找第一个图像卡作为适合物理设备
		__pickPhysicalDevice();
		__createLogicalDevice();
		__createSwapShain();
		__createImageViews();
		__createGraphicsPipelines();
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
		//注意删除顺序，先删除surface 后删除Instance
		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);

		//循环删除视图
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
		}
		vkDestroyDevice(m_Device, nullptr);
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}

	//创建实例对象
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

		//描述队列蔟中预定申请的队列个数,创建支持表现和显示功能
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
		//需要多个VkDeviceQueueCreateInfo结构来创建不同功能的队列。一个方式是针对不同功能的队列簇创建一个set集合确保队列簇的唯一性
		std::set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

		//分配队列优先级
		float QueuePriority = 1.0f;
		for (int queueFamiliy : UniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo QueueCreateInfo;
			QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfo.queueFamilyIndex = queueFamiliy;
			QueueCreateInfo.queueCount = 1;
			QueueCreateInfo.pQueuePriorities = &QueuePriority;
			QueueCreateInfos.push_back(QueueCreateInfo);
		}

		//指定使用的设备特性
		VkPhysicalDeviceFeatures DeviceFeatures = {};

		//创建逻辑设备
		VkDeviceCreateInfo DeviceCreateInfo = {};
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//添加指向队列创建信息的结构体和设备功能结构体:
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
		DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
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

		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

		//调用vkCreateDevice函数来创建实例化逻辑设备
		VkResult Result = vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, NULL, &m_Device);
		if (Result != VK_SUCCESS) throw std::runtime_error("failed to create logical device!");

		//获取指定队列族的队列句柄
		vkGetDeviceQueue(m_Device, Indices.PresentFamily, 0, &m_PresentQueue);
	}
	//判断是否支持gPU函数
	bool __isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		EQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);

		//是否支持交换链支持
		bool DeviceExtensionSupport = __checkDeviceExtensionSupport(vDevice);

		bool SwapChainAdequate = false;
		if (DeviceExtensionSupport)
		{
			ESwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
			//返回真，说明交换链的能力满足需要
			SwapChainAdequate = !SwapChainSupport.SurfaceFormats.empty() && !SwapChainSupport.PresentModes.empty();
		}
		return (Indices.isComplete() && DeviceExtensionSupport && SwapChainAdequate);
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
			//查找带有表现图像到窗口表面能力的队列族
			VkBool32 PresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(vDevice, i, m_WindowSurface, &PresentSupport);
			//根据队列簇中的队列数量和是否支持表现能力来确定使用的表现队列簇的索引
			if (QueueFamily.queueCount > 0 && PresentSupport)
			{
				Indices.PresentFamily = i;
			}
			if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags && VK_QUEUE_GRAPHICS_BIT)
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
		std::cerr << "validation layer:" << pCallbackData->pMessage << std::endl;
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

	//Surface
	void __createSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_WindowSurface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Window Surface!");
		}
	}

	//检测交换链是否支持：使用交换链必须保证VK_KHR_swapchain设备启用
	bool __checkDeviceExtensionSupport(VkPhysicalDevice vPhysicalDevice)
	{
		uint32_t ExtensionCount;
		vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, nullptr);

		//思想：将所需的扩展保存在一个集合，权举所有可用的扩展，将集合中扩展删除，若集合为空，则所需扩展均能得到满足
		std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
		vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data());

		std::set<std::string> RequireExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
		for (const auto& entension : AvailableExtensions)
		{
			RequireExtensions.erase(entension.extensionName);
		}

		return RequireExtensions.empty();
	}

	//检查交换链细节
	ESwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice vPhsicalDevice)
	{
		ESwapChainSupportDetails Details;
		//查询基础表面特性
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhsicalDevice, m_WindowSurface, &Details.Capablities);

		//查询表面支持的格式
		uint32_t FormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, nullptr);
		if (FormatCount != 0)
		{
			Details.SurfaceFormats.resize(FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, Details.SurfaceFormats.data());
		}

		//查询支持的呈现模式
		uint32_t PresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(vPhsicalDevice, m_WindowSurface, &PresentModeCount, nullptr);
		if (PresentModeCount != 0)
		{
			Details.PresentModes.resize(PresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, Details.PresentModes.data());
		}

		return Details;
	}

	//为交换链选择合适的设置：1. 表面格式 2. 呈现模式  3.交换范围
	//表面格式
	VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &vAvailablFormats)
	{
		//若是没有自己首选的模式，则直接返回设定的模式
		if (vAvailablFormats.size() == 1 && vAvailablFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& availableFormat : vAvailablFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		//大多数情况下，直接使用列表中的第一个格式也是非常不错的选择
		return vAvailablFormats[0];
	}

	//呈现模式，选择最佳模式
	VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR> vAvailablePresentModes)
	{
		VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;
		//使用三缓冲
		for (const auto& availablePresentMode : vAvailablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				BestMode = availablePresentMode;
			}
		}
		return BestMode;
	}

	//交换范围：具体表示图像的分辨率
	VkExtent2D __chooseSwapExtent(const VkSurfaceCapabilitiesKHR &vSurfaceCapabilities)
	{
		if (vSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return  vSurfaceCapabilities.currentExtent;
		}
		else {
			VkExtent2D ActualExtent = { WIDTH, HEIGHT };

			ActualExtent.width = std::max(vSurfaceCapabilities.minImageExtent.width,
				std::min(vSurfaceCapabilities.maxImageExtent.width,
					ActualExtent.width));
			ActualExtent.height = std::max(vSurfaceCapabilities.minImageExtent.height,
				std::min(vSurfaceCapabilities.maxImageExtent.height,
					ActualExtent.height));

			return ActualExtent;
		}
	}

	//创造交换链
	void __createSwapShain()
	{
		//查询可以支持交换链的
		ESwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(m_PhysicalDevice);
		VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.SurfaceFormats);
		VkPresentModeKHR   PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes);
		VkExtent2D         Extent = __chooseSwapExtent(SwapChainSupport.Capablities);

		//设置交换链中的图像个数，三倍缓冲
		uint32_t  ImageCount = SwapChainSupport.Capablities.minImageCount + 1;
		if (SwapChainSupport.Capablities.maxImageCount > 0 && ImageCount > SwapChainSupport.Capablities.maxImageCount)
		{
			ImageCount = SwapChainSupport.Capablities.maxImageCount;
		}

		//创建交换链图像的信息
		VkSwapchainCreateInfoKHR   SwapChainCreateInfo = {};
		SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapChainCreateInfo.surface = m_WindowSurface;
		SwapChainCreateInfo.minImageCount = ImageCount;
		SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapChainCreateInfo.imageExtent = Extent;
		SwapChainCreateInfo.imageArrayLayers = 1;  //指定每个图像所包含的层次
		SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  //指定在图像上进行怎样的操作

		//指定多个队列簇使用交换链图像的方式
		//这里通过图形队列在交换链图像上进行绘制操作，然后将图像提交给呈现队列进行显示
		EQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);
		uint32_t QueueFamilyIndices[] = { (uint32_t)Indices.GraphicsFamily, (uint32_t)Indices.PresentFamily };

		//图形和呈现不是同一个队列族，使用协同模式来避免处理图像所有权问题；否则就不能使用
		if (Indices.GraphicsFamily != Indices.PresentFamily)
		{
			SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapChainCreateInfo.queueFamilyIndexCount = 2;
			SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
		}
		else {
			SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapChainCreateInfo.queueFamilyIndexCount = 0;
			SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		//可以为交换链中的图像指定一个固定的变换操作
		SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;   //指定alpha通道是否被用来和窗口系统中的其它窗口进行混合操作

		//设置呈现模式
		SwapChainCreateInfo.presentMode = PresentMode;
		SwapChainCreateInfo.clipped = VK_TRUE;              //clipped成员变量被设置为VK_TRUE表示我们不关心被窗口系统中的其它窗口遮挡的像素的颜色
		SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;  //需要指定它，因为应用程序在运行过程中可能会失效，由于之前还没有创建任何一个交换链，将设置为VK_NULL_HANDLE即可。

		//创建交换链
		if (vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create SwapChain!");
		}

		//获取交换链图像句柄,先获取数量，然后分配空间，获取句柄
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, nullptr);
		m_SwapChainImages.resize(ImageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, m_SwapChainImages.data());
	}

	//为每一个交换链中图像创建基本的图像
	void __createImageViews()
	{
		//保存图像集大小
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		//循环迭代交换链图像
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo ImageViewCreateInfo = {};
			ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ImageViewCreateInfo.image = m_SwapChainImages[i];
			ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageViewCreateInfo.format = m_SwapChainImageFormat;

			//components字段调整颜色通道的最终映射逻辑
			ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			//subresourceRangle字段用于描述图像的使用目标是什么，以及可以被访问的有效区域
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ImageViewCreateInfo.subresourceRange.levelCount = 1;
			ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ImageViewCreateInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create ImageView!");
			}
		}
	}

	//读文件
	static std::vector<char> __readFile(const std::string& vFilename)
	{
		std::ifstream file(vFilename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
	}

	//将shader代码传递给渲染管线之前，必须将其封装到VkShaderModule对象中
	VkShaderModule __createShaderModule(const std::vector<char>& vShaderCode)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = vShaderCode.size();
		//Shadercode字节码是以字节为指定，但字节码指针是uint32_t类型，因此需要转换类型
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

		//创建vkShaderModule
		VkShaderModule ShaderModule;
		if (vkCreateShaderModule(m_Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule) !VK_SUCCESS)
		{
			throw std::runtime_error("failed to create ShaderModule!");
		}
		return ShaderModule;
	}
	void __createGraphicsPipelines()
	{
		auto VertShaderCode = __readFile("../Shader/Vert/shader.vert");
		auto FragShaderCode = __readFile("../Shader/Frag/shader.frag");

		VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

		//删除
		vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);

		//着色器创建：VkShaderModule对象只是字节码缓冲区的一个包装容器。着色器并没有彼此链接
		//通过VkPipelineShaderStageCreateInfo结构将着色器模块分配到管线中的顶点或者片段着色器阶段
		
	}
};
