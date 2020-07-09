#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>

namespace test3
{
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	//SDK通过请求VK_LAYER_LUNARG_standard_validaction层，
	//来隐式的开启有所关于诊断layers，
	//从而避免明确的指定所有的明确的诊断层。
	const std::vector<const char*> ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	//定义所需的设备扩展列表
	const std::vector<const char*> DeviceExtensions = {
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
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
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
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(vInstance, vCallback, pAllocator);
		}
	}


	class CTest
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
		VkRenderPass m_RenderPass;
		std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

		//创建指令缓冲对象之前，需要先创建指令池对象
		VkCommandPool m_CommandPool;
		//创建指令缓冲对象,用来记录绘制命令，由于绘制操作是在帧缓冲上进行的，需要为交换链中每一个图像分配一个指令缓冲对象
		//VkCommandBuffer m_CommandBuffer;
		//指令缓冲对象会在指令池对象被清除时自动被清除，不需要我们自己显式地清除它
		std::vector<VkCommandBuffer> m_CommandBuffers;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;

		//信号量
		VkSemaphore ImageAvailableSemaphore;  //准备渲染的信号量
		VkSemaphore RenderFinishedSemaphore;  //渲染结束信号量，准备进行呈现present

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;
		size_t m_CurrentFrame = 0;

		//许多驱动程序会在窗口大小改变后触发VK_ERROR_OUT_OF_DATE_KHR信息，
		//但这种触发并不可靠，所以最好添加一些代码来显式地在窗口大小改变时重建交换链。
		//添加一个新的变量来标记窗口大小是否发生改变：
		bool m_WindowResized = false;

		//创建窗口
		void __initWindow()
		{
			glfwInit();
			//由于GLFW库最初是为了OpenGL设计，需要显式设置GLFW阻止它自动创建OpenGL上下文
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
			//GLFW允许我们使用glfwSetWindowUserPointer将任意指针存储在窗体对象中，
			//因此可以指定静态类成员调用glfwGetWindowUserPointer返回原始的实例对象
			glfwSetWindowUserPointer(m_pWindow, this);
			//会在窗体发生大小变化的时候被事件回调
			glfwSetFramebufferSizeCallback(m_pWindow, CTest::__framebufferResizeCallback);
		}

		//回调函数,必须为静态函数，才能作为回调函数
		static void __framebufferResizeCallback(GLFWwindow* pWindow, int vWidth, int vHeight) {
			if (vWidth == 0 || vHeight == 0) return;

			auto app = reinterpret_cast<CTest*>(glfwGetWindowUserPointer(pWindow));
			app->m_WindowResized = true;
		}

		//创建vulkan实例
		void __initVulkan()
		{
			__createInstance();
			//__setupDebugCallBack();
			__createSurface();
			//创建实例之后，需要在系统中找到一个支持功能的显卡，查找第一个图像卡作为适合物理设备
			__pickPhysicalDevice();
			__createLogicalDevice();
			__createSwapShain();
			__createImageViews();
			__createRenderPass();
			__createGraphicsPipelines();
			__createFrameBuffers();
			__createCommandPool();
			__createCommandBuffers();
			__creatSemaphores(); //创造信号量
		}

		//重建交换链
		void  __recreateSwapChain()
		{
			int Width = 0, Height = 0;
			//当窗口最小化时，窗口的帧缓冲实际大小也为0，设置应用程序在窗口最小化后停止渲染，直到窗口重新可见时重建交换链
			if (Width == 0 || Height == 0)
			{
				glfwGetFramebufferSize(m_pWindow, &Width, &Height);
				glfwWaitEvents();
			}
			vkDeviceWaitIdle(m_Device);

			__createSwapShain();
			__createImageViews();         //图像视图是建立在交换链图像基础上的
			__createRenderPass();         //依赖于交换链图像的格式
			__createGraphicsPipelines();  //创建图形管线期间指定Viewport和scissor 矩形大小,改变了图像的大小
			__createFrameBuffers();		  //依赖于交换链图像
			__createCommandBuffers();	  //依赖于交换链图像
		}

		//循环渲染
		void __mainLoop()
		{
			while (!glfwWindowShouldClose(m_pWindow))
			{
				glfwPollEvents();
				//函数中所有的操作都是异步的。意味着当程序退出mainLoop，
				//绘制和呈现操作可能仍然在执行。所以清理该部分的资源是不友好的
				//为了解决这个问题，我们应该在退出mainLoop销毁窗体前等待逻辑设备的操作完成:
				__drawFrame();   //异步操作
			}
			vkDeviceWaitIdle(m_Device);
		}

		//为了确保重新创建相关的对象之前，老版本的对象被系统正确回收清理,需要重构Cleanup
		void __cleanUpSwapChain()
		{
			//删除帧缓冲
			for (size_t i = 0; i < m_SwapChainFrameBuffers.size(); i++)
			{
				vkDestroyFramebuffer(m_Device, m_SwapChainFrameBuffers[i], nullptr);
			}

			//选择借助vkFreeCommandBuffers函数清理已经存在的命令缓冲区。这种方式可以重用对象池中已经分配的命令缓冲区。
			vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

			vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
			vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

			//循环删除视图
			for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
			{
				vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
			}

			vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		}

		//退出
		void __cleanUp()
		{
			__cleanUpSwapChain();

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
				vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
			}

			//删除指令池对象
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

			vkDestroyDevice(m_Device, nullptr);
			//使用Vulkan API创建的对象也需要被清除，且应该在Vulkan实例清除之前被清除。
			if (enableValidationLayers)
			{
				destroyDebugUtilsMessengerEXT(m_Instance, m_CallBack, nullptr);
			}
			//注意删除顺序，先删除surface 后删除Instance
			vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
			vkDestroyInstance(m_Instance, nullptr);

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
			//CreateInfo.enabledExtensionCount = 0;

			//DebugInfo
			VkDebugUtilsMessengerCreateInfoEXT  CreateDebugInfo;

			//修改VkInstanceCreateInfo结构体，填充当前上下文已经开启的validation layers名称集合。
			if (enableValidationLayers)
			{
				CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
				CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
				__populateDebugMessengerCreateInfo(CreateDebugInfo);
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
					m_PhysicalDevice = device;
					break;
				}
			}

			if (m_PhysicalDevice == VK_NULL_HANDLE)
			{
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}

		//创建逻辑设备：
		//FUNC:主要与物理设备进行交互
		void __createLogicalDevice()
		{
			//创建队列
			EQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);

			//描述队列蔟中预定申请的队列个数,创建支持表现和显示功能
			std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
			//需要多个VkDeviceQueueCreateInfo结构来创建不同功能的队列。一个方式是针对不同功能的队列簇创建一个set集合确保队列簇的唯一性
			std::set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

			//优先级
			float QueuePriority = 1.0f;
			for (int queueFamily : UniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &QueuePriority;
				QueueCreateInfos.push_back(queueCreateInfo);
			}

			////指定使用的设备特性
			VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};

			////创建逻辑设备
			VkDeviceCreateInfo DeviceCreateInfo = {};
			DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			////添加指向队列创建信息的结构体和设备功能结构体:
			DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
			DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());

			DeviceCreateInfo.pEnabledFeatures = &PhysicalDeviceFeatures;

			DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
			DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

			////为设备开启validation layers
			if (enableValidationLayers) {
				DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
				DeviceCreateInfo.ppEnabledLayerNames = ValidationLayers.data();
			}
			else {
				DeviceCreateInfo.enabledLayerCount = 0;
			}

			if (vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS) {
				throw std::runtime_error("failed to create logical device!");
			}

			//获取指定队列族的队列句柄
			vkGetDeviceQueue(m_Device, Indices.GraphicsFamily, 0, &m_GraphicsQueue);
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

		void __populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& VCreateInfo) {
			VCreateInfo = {};
			VCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			VCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			VCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			VCreateInfo.pfnUserCallback = __debugCallBack;
		}

		//调用回调函数
		void __setupDebugCallBack()
		{
			if (!enableValidationLayers) return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			__populateDebugMessengerCreateInfo(createInfo);

			if (createDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_CallBack) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}

		//Surface
		void __createSurface()
		{
			if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_WindowSurface) != VK_SUCCESS) {
				throw std::runtime_error("failed to create window surface!");
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
			int Width, Height;
			glfwGetWindowSize(m_pWindow, &Width, &Height);

			if (vSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				return  vSurfaceCapabilities.currentExtent;
			}
			else {
				VkExtent2D ActualExtent = { Width, Height };

				ActualExtent.width = std::max(vSurfaceCapabilities.minImageExtent.width, std::min(vSurfaceCapabilities.maxImageExtent.width, ActualExtent.width));
				ActualExtent.height = std::max(vSurfaceCapabilities.minImageExtent.height, std::min(vSurfaceCapabilities.maxImageExtent.height, ActualExtent.height));
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
			SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;   //指定alpha通道是否被用来和窗口系统中的其它窗口进行混合操作
			SwapChainCreateInfo.preTransform = SwapChainSupport.Capablities.currentTransform;
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
			m_SwapChainImageFormat = SurfaceFormat.format;
			m_SwapChainExtent = Extent;
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
		static std::vector<char> __readFile(const std::string& filename) {
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("failed to open file!");
			}
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();
			return buffer;
		}

		//将shader代码传递给渲染管线之前，必须将其封装到VkShaderModule对象中
		VkShaderModule __createShaderModule(const std::vector<char>& vShaderCode)
		{
			VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
			ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			ShaderModuleCreateInfo.codeSize = vShaderCode.size();
			//Shadercode字节码是以字节为指定，但字节码指针是uint32_t类型，因此需要转换类型
			ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

			//创建vkShaderModule
			VkShaderModule ShaderModule;
			if (vkCreateShaderModule(m_Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create ShaderModule!");
			}
			return ShaderModule;
		}

		//创建渲染通道
		void __createRenderPass()
		{
			//附件描述
			VkAttachmentDescription ColorAttachmentDescription = {}; //这里只用一个颜色缓冲附件
			ColorAttachmentDescription.format = m_SwapChainImageFormat;  //指定颜色附件格式，这里与交换链中保持一致
			ColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;  //这里不做多重采样
			//渲染前和渲染后数据在对应附件的操作行为，应用颜色和深度数据
			ColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  //起始阶段以一个常量清理附件内容
			ColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //渲染的内容会存储在内存，并在之后进行读取操作
			//用于模板数
			ColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //图像在交换链中被呈现

			ColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			//子通道：一个单独的渲染通道可以由多个子通道组成。子通道是渲染操作的一个序列
			VkAttachmentReference ColorAttachmentRef = {};
			ColorAttachmentRef.attachment = 0;  //通过附件描述符集合中的索引来持有,这里只有一个颜色附件
			ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			//子通道
			VkSubpassDescription SubpassDescription = {};
			SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //指定graphics subpass图形子通道
			SubpassDescription.colorAttachmentCount = 1;
			SubpassDescription.pColorAttachments = &ColorAttachmentRef;  //指定颜色附件引用。附件在数组中的索引直接从片段着色器引用，其layout(location = 0) out vec4 outColor 指令!

			//创建渲染管道
			VkRenderPassCreateInfo RenderPassCreateInfo = {};
			RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPassCreateInfo.attachmentCount = 1;
			RenderPassCreateInfo.pAttachments = &ColorAttachmentDescription;
			RenderPassCreateInfo.subpassCount = 1;
			RenderPassCreateInfo.pSubpasses = &SubpassDescription;

			//子通道依赖：渲染通道中的子通道会自动处理布局的变换。这些变换通过子通道的依赖关系进行控制
			VkSubpassDependency SubpassDependency = {};
			SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;  //指定要依赖关系
			SubpassDependency.dstSubpass = 0;  //从属子通道的索引
			SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //指定等待的操作，这里等待交换链完成对应图像的读取操作。这可以通过等待颜色附件输出的阶段来实现
			SubpassDependency.srcAccessMask = 0;
			//在颜色附件阶段的操作及涉及颜色附件的读取和写入的操作应该等待。这些设置将阻止转换发生
			SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			RenderPassCreateInfo.dependencyCount = 1;
			RenderPassCreateInfo.pDependencies = &SubpassDependency;

			if (vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		//创建图形管线
		void __createGraphicsPipelines()
		{
			auto VertShaderCode = __readFile("vert.spv");
			auto FragShaderCode = __readFile("frag.spv");

			VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
			VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

			//着色器创建：VkShaderModule对象只是字节码缓冲区的一个包装容器。着色器并没有彼此链接
			//通过VkPipelineShaderStageCreateInfo结构将着色器模块分配到管线中的顶点或者片段着色器阶段

			//顶点
			VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
			VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VertShaderStageInfo.module = VertShaderModule;
			VertShaderStageInfo.pName = "main";

			//片段
			VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
			FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			FragShaderStageInfo.module = FragShaderModule;
			FragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

			//顶点信息
			VkPipelineVertexInputStateCreateInfo  PipelineVertexInputStateCreateInfo = {};
			PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
			PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
			PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
			PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

			//顶点数据的集合图元拓扑结构和是否启用顶点索重新开始图元
			VkPipelineInputAssemblyStateCreateInfo  PipelineInputAssemblyStateCreateInfo = {};
			PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  //三点共面，顶点不共用
			PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

			//视图
			VkViewport Viewport = {};
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;
			Viewport.width = (float)m_SwapChainExtent.width;
			Viewport.height = (float)m_SwapChainExtent.height;
			Viewport.minDepth = 0.0f;
			Viewport.maxDepth = 1.0f;

			//定义裁剪矩形覆盖到整体图像
			VkRect2D Scissor = {};
			Scissor.offset = { 0, 0 };
			Scissor.extent = m_SwapChainExtent;

			//将viewport和Scissor联合使用
			VkPipelineViewportStateCreateInfo  PipelineViewportStateCreateInfo = {};
			PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;;
			PipelineViewportStateCreateInfo.viewportCount = 1;
			PipelineViewportStateCreateInfo.pViewports = &Viewport;
			PipelineViewportStateCreateInfo.scissorCount = 1;
			PipelineViewportStateCreateInfo.pScissors = &Scissor;

			//光栅化:通过顶点着色器以及具体的几何算法将顶点塑形，并将图形传递到片段着色器进行着色
			//执行深度测试，面剪切，裁剪测试
			VkPipelineRasterizationStateCreateInfo  PipelineRasterizationStateCreateInfo = {};
			PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;;
			PipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;         //vK_TRUE:超过远近裁剪面的片元进行收敛，而不是舍弃
			PipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;  //VK_TRUE：几何图元永远不会传递到光栅化
			PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;  //决定几何产生图片的内容，多边形填充
			PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;                    //根据片元的数量描述线的宽度。最大的线宽支持取决于硬件，任何大于1.0的线宽需要开启GPU的wideLines特性支持
			PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;    //指定剪裁面的类型方式
			PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; //front-facing面的顶点的顺序，可以是顺时针也可以是逆时针
			PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
			PipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
			PipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
			PipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

			//重采样
			VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo = {};
			PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
			PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			PipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // Optional
			PipelineMultisampleStateCreateInfo.pSampleMask = nullptr; // Optional
			PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
			PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

			//混色
			VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {};
			PipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
			PipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			PipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			PipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			PipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			//所有帧缓冲区的引用
			VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {};
			PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
			PipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
			PipelineColorBlendStateCreateInfo.attachmentCount = 1;
			PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
			PipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
			PipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
			PipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
			PipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

			//管道布局
			VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
			PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutCreateInfo.setLayoutCount = 0; // Optional
			PipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
			PipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
			PipelineLayoutCreateInfo.pPushConstantRanges = 0; // Optional

			if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			//创建管线
			VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
			GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			GraphicsPipelineCreateInfo.stageCount = 2;
			GraphicsPipelineCreateInfo.pStages = ShaderStages;
			GraphicsPipelineCreateInfo.pVertexInputState = &PipelineVertexInputStateCreateInfo;
			GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
			GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
			GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
			GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
			GraphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional
			GraphicsPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
			GraphicsPipelineCreateInfo.pDynamicState = nullptr; // Optional
			GraphicsPipelineCreateInfo.layout = m_PipelineLayout;   //是句柄不是结构体
			GraphicsPipelineCreateInfo.renderPass = m_RenderPass;
			GraphicsPipelineCreateInfo.subpass = 0;
			//可以通过已经存在的管线创建新的图形管线（功能相同，节约内存消耗）
			GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			GraphicsPipelineCreateInfo.basePipelineIndex = -1;

			//创建图形管线
			if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			//删除
			vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
			vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
		}

		//创建帧缓冲
		void __createFrameBuffers()
		{
			//动态调整用于保存framebuffers的容器大小
			m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

			//迭代图像视图并通过他们建立对应FrameBuffer
			for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
			{
				VkImageView Attachments[] = { m_SwapChainImageViews[i] };

				VkFramebufferCreateInfo FrameBufferCreateInfo = {};
				FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				FrameBufferCreateInfo.renderPass = m_RenderPass;
				FrameBufferCreateInfo.attachmentCount = 1;
				FrameBufferCreateInfo.pAttachments = Attachments;
				FrameBufferCreateInfo.width = m_SwapChainExtent.width;
				FrameBufferCreateInfo.height = m_SwapChainExtent.height;
				FrameBufferCreateInfo.layers = 1;

				if (vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, &m_SwapChainFrameBuffers[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to create Frame Buffer!");
				}
			}
		}

		//创建指令池
		void __createCommandPool()
		{
			//每个指令池对象分配的指令缓冲对象只能提交给一个特定类型的队列，这里使用绘制指令
			EQueueFamilyIndices QueueFamilyIndices = __findQueueFamilies(m_PhysicalDevice);

			VkCommandPoolCreateInfo CommandPoolCreateInfo = {};
			CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily;
			CommandPoolCreateInfo.flags = 0;

			if (vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create Command Pool!");
			}
		}

		//创建指令缓冲对象
		void __createCommandBuffers()
		{
			m_CommandBuffers.resize(m_SwapChainFrameBuffers.size());

			VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
			CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBufferAllocateInfo.commandPool = m_CommandPool;
			CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;   //可以被提交到队列进行执行，但不能被其它指令缓冲对象调用。
			CommandBufferAllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

			if (vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}

			//记录指令到指令缓冲
			for (size_t i = 0; i < m_CommandBuffers.size(); i++)
			{
				VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
				CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  //在指令缓冲等待执行时，仍然可以提交这一指令缓冲
				CommandBufferBeginInfo.pInheritanceInfo = nullptr;

				if (vkBeginCommandBuffer(m_CommandBuffers[i], &CommandBufferBeginInfo) != VK_SUCCESS) {
					throw std::runtime_error("failed to begin recording command buffer!");
				}

				//开始渲染流程
				VkRenderPassBeginInfo RenderPassBeginInfo = {};
				RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				RenderPassBeginInfo.renderPass = m_RenderPass;   //指定渲染流程对象
				RenderPassBeginInfo.framebuffer = m_SwapChainFrameBuffers[i]; //指定帧缓冲对象
				//渲染区域，这里与附着保持一致
				RenderPassBeginInfo.renderArea.offset = { 0,0 };
				RenderPassBeginInfo.renderArea.extent = m_SwapChainExtent;
				//清除值
				VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				RenderPassBeginInfo.clearValueCount = 1;
				RenderPassBeginInfo.pClearValues = &ClearColor;

				//参数：1.指令缓冲对象  2.渲染流程信息  3.所有要执行的指令都在主要指令缓冲中，没有辅助指令缓冲需要执行
				vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				//绑定图形管线:第二个参数用于指定管线对象是图形管线还是计算管线
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

				//提交绘制操作到指令缓冲，
				//参数：1.指令缓冲对象 2.顶点缓冲，三个顶点绘制一个三角形 3.1：表示实例渲染 4.定义着色器变量gl_VertexIndex的值 5.定义着色器变量gl_InstanceIndex的值
				vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

				//结束渲染流程
				vkCmdEndRenderPass(m_CommandBuffers[i]);

				//结束记录指令到指令缓冲
				if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to record command buffer!");
				}
			}
		}

		//绘制步骤：需要同步操作，栅栏（用于应用程序与渲染操作进行同步）或者信号量（用于在命令队列内或者跨命令队列同步操作）实现同步
		//1.首先，从交换链中获取一图像
		//2.从帧缓冲中使用作为附件的图像执行命令缓冲区中的命令
		//3.渲染后的图像返回交换链
		void __drawFrame()
		{
			vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

			uint32_t ImageIndex;
			//重建交换链
			VkResult Result = vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
			if (Result == VK_ERROR_OUT_OF_DATE_KHR)  //VK_ERROR_OUT_OF_DATE_KHR：交换链不能继续使用。通常发生在窗口大小改变后
			{
				__recreateSwapChain();
				return;
			}
			else if (Result != VK_SUCCESS &&  Result != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("failed to acquire SwapChain Image!");
			}

			//让CPU在当前位置被阻塞掉，然后一直等待到它接受的Fence变为signaled的状态
			//vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
			//1.从交换链中获取一图像
		
			//获取命令缓冲
			//vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
			if (m_ImagesInFlight[ImageIndex] != VK_NULL_HANDLE) {
				vkWaitForFences(m_Device, 1, &m_ImagesInFlight[ImageIndex], VK_TRUE, UINT64_MAX);
			}
			m_ImagesInFlight[ImageIndex] = m_InFlightFences[m_CurrentFrame];

			//2.提交命令缓冲区
			VkSubmitInfo SubmitInfo = {};
			SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
			VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			SubmitInfo.waitSemaphoreCount = 1;
			//执行开始之前要等待的哪个信号量及要等待的通道的哪个阶段
			SubmitInfo.pWaitSemaphores = WaitSemaphores;
			SubmitInfo.pWaitDstStageMask = WaitStages;
			//指定哪个命令缓冲区被实际提交执行，这里为提交命令缓冲区，它将我们刚获取的交换链图像做为颜色附件进行绑定。
			SubmitInfo.commandBufferCount = 1;
			SubmitInfo.pCommandBuffers = &m_CommandBuffers[ImageIndex];

			//指定了当命令缓冲区执行结束向哪些信号量发出信号。根据需要使用renderFinishedSemaphore
			VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
			SubmitInfo.signalSemaphoreCount = 1;
			SubmitInfo.pSignalSemaphores = SignalSemaphores;

			vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

			//向图像队列提交命令缓冲区
			if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer!");
			}

			//3.将结果提交到交换链，使其最终显示在屏幕上
			VkPresentInfoKHR PresentInfo = {};
			PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			PresentInfo.waitSemaphoreCount = 1;
			//指定在进行presentation之前要等待的信号量
			PresentInfo.pWaitSemaphores = SignalSemaphores;

			//指定用于提交图像的交换链和每个交换链图像索引。大多数情况下仅一个
			VkSwapchainKHR SwapChains[] = { m_SwapChain };
			PresentInfo.swapchainCount = 1;
			PresentInfo.pSwapchains = SwapChains;
			PresentInfo.pImageIndices = &ImageIndex;
			//PresentInfo.pResults = nullptr;

			//不完全匹配时重建交换链
		    //提交请求呈现交换链中的图像
			Result = vkQueuePresentKHR(m_PresentQueue, &PresentInfo);
			if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_WindowResized) {
				m_WindowResized = false;
				__recreateSwapChain();
			}
			else if (Result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}

			//开始绘制下一帧之前明确的等待presentation完成:
			//vkQueueWaitIdle(m_PresentQueue);

			m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		//创建信号量
		void __creatSemaphores()
		{
			m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			m_ImagesInFlight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
					vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
					vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create synchronization objects for a frame!");
				}
			}
		}
	};
}