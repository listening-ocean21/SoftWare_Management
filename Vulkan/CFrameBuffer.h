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

//SDKͨ������VK_LAYER_LUNARG_standard_validaction�㣬
//����ʽ�Ŀ��������������layers��
//�Ӷ�������ȷ��ָ�����е���ȷ����ϲ㡣
const std::vector<const char*> ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

//����������豸��չ�б�
const std::vector<const char*> DeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//����������ñ�����ָ��Ҫ���õ�layers�Լ��Ƿ�������
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//�����Vulkanʵ�ֿ��ܶԴ���ϵͳ������֧�֣����Ⲣ����ζ������ƽ̨��Vulkanʵ�ֶ�֧��ͬ��������
//��Ҫ��չisDeviceSuitable������ȷ���豸���������Ǵ����ı�������ʾͼ��
struct SQueueFamilyIndices
{	//֧�ֻ���ָ��Ķ��дغ�֧�ֱ��ֵĶ��д�
	int GraphicsFamily = -1;
	int PresentFamily = -1;

	bool isComplete()
	{
		return (GraphicsFamily >= 0 && PresentFamily >= 0);
	}
};

//��齻������ϸ�ڣ�1.surface����  2.surface��ʽ 3.���õĳ���ģʽ
struct SSwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR Capablities;
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> PresentModes;
};
//��д��ṹ��VkDebugUtilsMessengerCreateInfoEXT ��Ϣ�󣬽�����Ϊһ����������vkCreateDebugUtilsMessengerEXT����������VkDebugUtilsMessengerEXT����
//������vkCreateDebugUtilsMessengerEXT������һ����չ���������ᱻVulkan���Զ����أ���Ҫʹ��vkGetInstanceProcAddr����������
//FUNCTION:һ������������Ҫ��Ϊ�˼���vkCreateDebugUtilsMessengerEXT����
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


class CFrameBuffer
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
	GLFWwindow * pWindow;
	VkInstance m_Instance;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkDebugUtilsMessengerEXT m_CallBack;

	VkSurfaceKHR m_WindowSurface;

	//�洢��������
	VkSwapchainKHR m_SwapChain;
	//��ȡ������ͼ���ͼ����
   //������ͼ���ɽ������Լ����𴴽������ڽ��������ʱ�Զ������������Ҫ�����Լ����д������������
	std::vector<VkImage> m_SwapChainImages;
	VkFormat   m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
	//�洢ͼ����ͼ�ľ����
	std::vector<VkImageView> m_SwapChainImageViews;
	VkRenderPass m_RenderPass;
	
	std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

	//����ָ������֮ǰ����Ҫ�ȴ���ָ��ض���
	VkCommandPool m_CommandPool;
	//����ָ������,������¼����������ڻ��Ʋ�������֡�����Ͻ��еģ���ҪΪ��������ÿһ��ͼ�����һ��ָ������
	VkCommandBuffer m_CommandBuffer;
	//ָ���������ָ��ض������ʱ�Զ������������Ҫ�����Լ���ʽ�������
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;
	//��������
	void __initWindow()
	{
		glfwInit();
		//����GLFW�������Ϊ��OpenGL��ƣ���Ҫ��ʽ����GLFW��ֹ���Զ�����OpenGL������
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//��ֹ���ڴ�С�ı� 
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
	}

	//����vulkanʵ��
	void __initVulkan()
	{
		__createInstance();
		__setupDebugCallBack();
		__createSurface();
		//����ʵ��֮����Ҫ��ϵͳ���ҵ�һ��֧�ֹ��ܵ��Կ������ҵ�һ��ͼ����Ϊ�ʺ������豸
		__pickPhysicalDevice();
		__createLogicalDevice();
		__createSwapShain();
		__createImageViews();
		__createRenderPass();
		__createGraphicsPipelines();
		__createFrameBuffer();
		__createCommandPool();
		__createCommandBuffers();

	}

	//ѭ����Ⱦ
	void __mainLoop()
	{
		while (!glfwWindowShouldClose(pWindow))
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
		//ע��ɾ��˳����ɾ��surface ��ɾ��Instance
		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		//ѭ��ɾ����ͼ
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
		}
		//ɾ��֡����
		for (size_t i = 0; i < m_SwapChainFrameBuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_Device, m_SwapChainFrameBuffers[i], nullptr);
		}
		//ɾ��ָ��ض���
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		glfwDestroyWindow(pWindow);
		glfwTerminate();
	}

	//����ʵ������
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
		SQueueFamilyIndices  Indices = __findQueueFamilies(m_PhysicalDevice);

		//������������Ԥ������Ķ��и���,����֧�ֱ��ֺ���ʾ����
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
		//��Ҫ���VkDeviceQueueCreateInfo�ṹ��������ͬ���ܵĶ��С�һ����ʽ����Բ�ͬ���ܵĶ��дش���һ��set����ȷ�����дص�Ψһ��
		std::set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

		//����������ȼ�
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

		//ָ��ʹ�õ��豸����
		VkPhysicalDeviceFeatures DeviceFeatures = {};

		//�����߼��豸
		VkDeviceCreateInfo DeviceCreateInfo = {};
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//���ָ����д�����Ϣ�Ľṹ����豸���ܽṹ��:
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
		DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
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

		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

		//����vkCreateDevice����������ʵ�����߼��豸
		VkResult Result = vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, NULL, &m_Device);
		if (Result != VK_SUCCESS) throw std::runtime_error("failed to create logical device!");

		//��ȡָ��������Ķ��о��
		vkGetDeviceQueue(m_Device, Indices.PresentFamily, 0, &m_PresentQueue);
	}
	//�ж��Ƿ�֧��gPU����
	bool __isDeviceSuitable(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);

		//�Ƿ�֧�ֽ�����֧��
		bool DeviceExtensionSupport = __checkDeviceExtensionSupport(vDevice);

		bool SwapChainAdequate = false;
		if (DeviceExtensionSupport)
		{
			SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
			//�����棬˵��������������������Ҫ
			SwapChainAdequate = !SwapChainSupport.SurfaceFormats.empty() && !SwapChainSupport.PresentModes.empty();
		}
		return (Indices.isComplete() && DeviceExtensionSupport && SwapChainAdequate);
	}

	//����豸��֧�ֵĶ�����
	SQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vDevice)
	{
		SQueueFamilyIndices  Indices;

		//�豸������ĸ���
		uint32_t QueueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

		//�ҵ�һ��֧��VK_QUEUE_GRAPHICS_BIT�Ķ��д�
		int i = 0;
		for (const auto& QueueFamily : QueueFamilies)
		{
			//���Ҵ��б���ͼ�񵽴��ڱ��������Ķ�����
			VkBool32 PresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(vDevice, i, m_WindowSurface, &PresentSupport);
			//���ݶ��д��еĶ����������Ƿ�֧�ֱ���������ȷ��ʹ�õı��ֶ��дص�����
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

	//������������У���layers�Ƿ����
	bool __checkValidationLayerSupport()
	{
		uint32_t LayerCount;
		//��һ�Σ����ڶ�����������Ϊ NULL�����ڵ�һ������ instanceLayerCount �з��ز��������
		vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
		//�ڵڶ��ε����У����ǽ��ڶ�����������Ϊ NULL���������䴫��һ�� VkLayerProperties �� vector��������õ���ϸ������Ϣ���ݵ����н��д洢��
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
		std::cerr << "validation layer:" << pCallbackData->pMessage << std::endl;
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

	//Surface
	void __createSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, pWindow, nullptr, &m_WindowSurface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Window Surface!");
		}
	}

	//��⽻�����Ƿ�֧�֣�ʹ�ý��������뱣֤VK_KHR_swapchain�豸����
	bool __checkDeviceExtensionSupport(VkPhysicalDevice vPhysicalDevice)
	{
		uint32_t ExtensionCount;
		vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, nullptr);

		//˼�룺���������չ������һ�����ϣ�Ȩ�����п��õ���չ������������չɾ����������Ϊ�գ���������չ���ܵõ�����
		std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
		vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data());

		std::set<std::string> RequireExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
		for (const auto& entension : AvailableExtensions)
		{
			RequireExtensions.erase(entension.extensionName);
		}

		return RequireExtensions.empty();
	}

	//��齻����ϸ��
	SSwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice vPhsicalDevice)
	{
		SSwapChainSupportDetails Details;
		//��ѯ������������
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhsicalDevice, m_WindowSurface, &Details.Capablities);

		//��ѯ����֧�ֵĸ�ʽ
		uint32_t FormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, nullptr);
		if (FormatCount != 0)
		{
			Details.SurfaceFormats.resize(FormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, Details.SurfaceFormats.data());
		}

		//��ѯ֧�ֵĳ���ģʽ
		uint32_t PresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(vPhsicalDevice, m_WindowSurface, &PresentModeCount, nullptr);
		if (PresentModeCount != 0)
		{
			Details.PresentModes.resize(PresentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(vPhsicalDevice, m_WindowSurface, &FormatCount, Details.PresentModes.data());
		}

		return Details;
	}

	//Ϊ������ѡ����ʵ����ã�1. �����ʽ 2. ����ģʽ  3.������Χ
	//�����ʽ
	VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &vAvailablFormats)
	{
		//����û���Լ���ѡ��ģʽ����ֱ�ӷ����趨��ģʽ
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

		//���������£�ֱ��ʹ���б��еĵ�һ����ʽҲ�Ƿǳ������ѡ��
		return vAvailablFormats[0];
	}

	//����ģʽ��ѡ�����ģʽ
	VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR> vAvailablePresentModes)
	{
		VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;
		//ʹ��������
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

	//������Χ�������ʾͼ��ķֱ���
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

	//���콻����
	void __createSwapShain()
	{
		//��ѯ����֧�ֽ�������
		SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(m_PhysicalDevice);
		VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.SurfaceFormats);
		VkPresentModeKHR   PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes);
		VkExtent2D         Extent = __chooseSwapExtent(SwapChainSupport.Capablities);

		//���ý������е�ͼ���������������
		uint32_t  ImageCount = SwapChainSupport.Capablities.minImageCount + 1;
		if (SwapChainSupport.Capablities.maxImageCount > 0 && ImageCount > SwapChainSupport.Capablities.maxImageCount)
		{
			ImageCount = SwapChainSupport.Capablities.maxImageCount;
		}

		//����������ͼ�����Ϣ
		VkSwapchainCreateInfoKHR   SwapChainCreateInfo = {};
		SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapChainCreateInfo.surface = m_WindowSurface;
		SwapChainCreateInfo.minImageCount = ImageCount;
		SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapChainCreateInfo.imageExtent = Extent;
		SwapChainCreateInfo.imageArrayLayers = 1;  //ָ��ÿ��ͼ���������Ĳ��
		SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  //ָ����ͼ���Ͻ��������Ĳ���

		//ָ��������д�ʹ�ý�����ͼ��ķ�ʽ
		//����ͨ��ͼ�ζ����ڽ�����ͼ���Ͻ��л��Ʋ�����Ȼ��ͼ���ύ�����ֶ��н�����ʾ
		SQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);
		uint32_t QueueFamilyIndices[] = { (uint32_t)Indices.GraphicsFamily, (uint32_t)Indices.PresentFamily };

		//ͼ�κͳ��ֲ���ͬһ�������壬ʹ��Эͬģʽ�����⴦��ͼ������Ȩ���⣻����Ͳ���ʹ��
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

		//����Ϊ�������е�ͼ��ָ��һ���̶��ı任����
		SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;   //ָ��alphaͨ���Ƿ������ʹ���ϵͳ�е��������ڽ��л�ϲ���

		//���ó���ģʽ
		SwapChainCreateInfo.presentMode = PresentMode;
		SwapChainCreateInfo.clipped = VK_TRUE;              //clipped��Ա����������ΪVK_TRUE��ʾ���ǲ����ı�����ϵͳ�е����������ڵ������ص���ɫ
		SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;  //��Ҫָ��������ΪӦ�ó��������й����п��ܻ�ʧЧ������֮ǰ��û�д����κ�һ����������������ΪVK_NULL_HANDLE���ɡ�

		//����������
		if (vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create SwapChain!");
		}

		//��ȡ������ͼ����,�Ȼ�ȡ������Ȼ�����ռ䣬��ȡ���
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, nullptr);
		m_SwapChainImages.resize(ImageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, m_SwapChainImages.data());
	}

	//Ϊÿһ����������ͼ�񴴽�������ͼ��
	void __createImageViews()
	{
		//����ͼ�񼯴�С
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		//ѭ������������ͼ��
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo ImageViewCreateInfo = {};
			ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ImageViewCreateInfo.image = m_SwapChainImages[i];
			ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageViewCreateInfo.format = m_SwapChainImageFormat;

			//components�ֶε�����ɫͨ��������ӳ���߼�
			ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			//subresourceRangle�ֶ���������ͼ���ʹ��Ŀ����ʲô���Լ����Ա����ʵ���Ч����
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

	//���ļ�
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

	//��shader���봫�ݸ���Ⱦ����֮ǰ�����뽫���װ��VkShaderModule������
	VkShaderModule __createShaderModule(const std::vector<char>& vShaderCode)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = vShaderCode.size();
		//Shadercode�ֽ��������ֽ�Ϊָ�������ֽ���ָ����uint32_t���ͣ������Ҫת������
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

		//����vkShaderModule
		VkShaderModule ShaderModule;
		if (vkCreateShaderModule(m_Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule) !VK_SUCCESS)
		{
			throw std::runtime_error("failed to create ShaderModule!");
		}
		return ShaderModule;
	}

	//������Ⱦͨ��
	void __createRenderPass()
	{
		//��������
		VkAttachmentDescription ColorAttachmentDescription = {}; //����ֻ��һ����ɫ���帽��
		ColorAttachmentDescription.format = m_SwapChainImageFormat;  //ָ����ɫ������ʽ�������뽻�����б���һ��
		ColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;  //���ﲻ�����ز���
		//��Ⱦǰ����Ⱦ�������ڶ�Ӧ�����Ĳ�����Ϊ��Ӧ����ɫ���������
		ColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  //��ʼ�׶���һ����������������
		ColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //��Ⱦ�����ݻ�洢���ڴ棬����֮����ж�ȡ����
		//����ģ����
		ColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		ColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ͼ���ڽ������б�����

		//��ͨ����һ����������Ⱦͨ�������ɶ����ͨ����ɡ���ͨ������Ⱦ������һ������
		VkAttachmentReference ColorAttachmentRef = {};
		ColorAttachmentRef.attachment = 0;  //ͨ�����������������е�����������,����ֻ��һ����ɫ����
		ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
		
		//��ͨ��
		VkSubpassDescription SubpassDescription = {};
		SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //ָ��graphics subpassͼ����ͨ��
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pColorAttachments = &ColorAttachmentRef;  //ָ����ɫ�������á������������е�����ֱ�Ӵ�Ƭ����ɫ�����ã���layout(location = 0) out vec4 outColor ָ��!

		//������Ⱦ�ܵ�
		VkRenderPassCreateInfo RrenderPassCreateInfo = {};
		RrenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RrenderPassCreateInfo.attachmentCount = 1;
		RrenderPassCreateInfo.pAttachments = &ColorAttachmentDescription;
		RrenderPassCreateInfo.subpassCount = 1;
		RrenderPassCreateInfo.pSubpasses = &SubpassDescription;

		if (vkCreateRenderPass(m_Device, &RrenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}

	}

	//����ͼ�ι���
	void __createGraphicsPipelines()
	{
		auto VertShaderCode = __readFile("../Shader/Vert/shader.vert");
		auto FragShaderCode = __readFile("../Shader/Frag/shader.frag");

		VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
		VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

		//ɾ��
		vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);

		//��ɫ��������VkShaderModule����ֻ���ֽ��뻺������һ����װ��������ɫ����û�б˴�����
		//ͨ��VkPipelineShaderStageCreateInfo�ṹ����ɫ��ģ����䵽�����еĶ������Ƭ����ɫ���׶�

		//������Ϣ
		VkPipelinSVertexInputStateCreateInfo  PipelinSVertexInputStateCreateInfo = {};
		PipelinSVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelinSVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		PipelinSVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		PipelinSVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		PipelinSVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

		//��ͼ
		VkViewport Viewport = {};
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = (float)m_SwapChainExtent.width;
		Viewport.height = (float)m_SwapChainExtent.height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		//����ü����θ��ǵ�����ͼ��
		VkRect2D Scissor = {};
		Scissor.offset = { 0, 0 };
		Scissor.extent = m_SwapChainExtent;

		//��viewport��Scissor����ʹ��
		VkPipelineViewportStateCreateInfo  PipelineViewportStateCreateInfo = {};
		PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;;
		PipelineViewportStateCreateInfo.viewportCount = 1;
		PipelineViewportStateCreateInfo.pViewports = &Viewport;
		PipelineViewportStateCreateInfo.scissorCount = 1;
		PipelineViewportStateCreateInfo.pScissors = &Scissor;

		//��դ��:ͨ��������ɫ���Լ�����ļ����㷨���������Σ�����ͼ�δ��ݵ�Ƭ����ɫ��������ɫ
		//ִ����Ȳ��ԣ�����У��ü�����
		VkPipelineRasterizationStateCreateInfo  PipelineRasterizationStateCreateInfo = {};
		PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;;
		PipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;         //vK_TRUE:����Զ���ü����ƬԪ��������������������
		PipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;  //VK_TRUE������ͼԪ��Զ���ᴫ�ݵ���դ��
		PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;  //�������β���ͼƬ�����ݣ���������
		PipelineRasterizationStateCreateInfo.lineWidth = 1.0f; //����ƬԪ�����������ߵĿ�ȡ������߿�֧��ȡ����Ӳ�����κδ���1.0���߿���Ҫ����GPU��wideLines����֧��
		PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; //ָ������������ͷ�ʽ
		PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;  //front-facing��Ķ����˳�򣬿�����˳ʱ��Ҳ��������ʱ��
		PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		PipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		PipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		PipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

		//�ز���
		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo = {};
		PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		PipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // Optional
		PipelineMultisampleStateCreateInfo.pSampleMask = nullptr; // Optional
		PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
		PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional
																		  
		//��ɫ
		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {};
		PipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
		PipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		PipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		PipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		PipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
																		  
		//����֡������������
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

		//�ܵ�����
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 0; // Optional
		PipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
		PipelineLayoutCreateInfo.pPushConstantRanges = 0; // Optional

		if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		//�������ݵļ���ͼԪ���˽ṹ���Ƿ����ö��������¿�ʼͼԪ
		VkPipelineInputAssemblyStateCreateInfo  PipelineInputAssemblyStateCreateInfo = {};
		PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  //���㹲�棬���㲻����
		PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		//��������
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo;
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.pStages = PipelinSVertexInputStateCreateInfo;
		GraphicsPipelineCreateInfo.pVertexInputState = &PipelinSVertexInputStateCreateInfo;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional
		GraphicsPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState = nullptr; // Optional
		GraphicsPipelineCreateInfo.layout = m_PipelineLayout;   //�Ǿ�����ǽṹ��
		GraphicsPipelineCreateInfo.renderPass = m_RenderPass;
		GraphicsPipelineCreateInfo.subpass = 0;
		//����ͨ���Ѿ����ڵĹ��ߴ����µ�ͼ�ι��ߣ�������ͬ����Լ�ڴ����ģ�
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;

		//����ͼ�ι���
		if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}

	//����֡����
	void __createFrameBuffer()
	{
		//��̬�������ڱ���framebuffers��������С
		m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

		//����ͼ����ͼ��ͨ�����ǽ�����ӦFrameBuffer
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
		{
			VkImageView Attachments[] = { m_SwapChainImageViews[i] };

			VkFramebufferCreateInfo FrameBufferCreateInfo = {};
			FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FrameBufferCreateInfo.renderPass = m_RenderPass;
			FrameBufferCreateInfo.attachmentCount = 1;
			FrameBufferCreateInfo.pAttachments = m_Attachment;
			FrameBufferCreateInfo.width = m_SwapChainExtent.width;
			FrameBufferCreateInfo.height = m_SwapChainExtent.height;
			FrameBufferCreateInfo.layers = 1;

			if (vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, &m_SwapChainFrameBuffers[i) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create Frame Buffer!");
			}
		}

	}

	//����ָ���
	void __createCommandPool()
	{
		//ÿ��ָ��ض�������ָ������ֻ���ύ��һ���ض����͵Ķ��У�����ʹ�û���ָ��
		SQueueFamilyIndices QueueFamilyIndices = __findQueueFamilies(m_PhysicalDevice);
		VkCommandPoolCreateInfo CommandPoolCreateInfo = {};

		CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily;
		CommandPoolCreateInfo.flags = 0;

		if (vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Command Pool!");
		}
	}

	//����ָ������
	void __createCommandBuffers()
	{
		m_CommandBuffers.resize(m_SwapChainFrameBuffers.size());

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = m_CommandPool; 
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;   //���Ա��ύ�����н���ִ�У������ܱ�����ָ��������á�
		CommandBufferAllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		//��¼ָ�ָ���
		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
			CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  //��ָ���ȴ�ִ��ʱ����Ȼ�����ύ��һָ���
			CommandBufferBeginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(m_CommandBuffers[i], &CommandBufferBeginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			//��ʼ��Ⱦ����
			VkRenderPassBeginInfo RenderPassBeginInfo = {};
			RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			RenderPassBeginInfo.renderPass = m_RenderPass;   //ָ����Ⱦ���̶���
			RenderPassBeginInfo.framebuffer = m_SwapChainFrameBuffers[i]; //ָ��֡�������
			//��Ⱦ���������븽�ű���һ��
			RenderPassBeginInfo.renderArea.offset = { 0,0 };
			RenderPassBeginInfo.renderArea.extent = m_SwapChainExtent;
			//���ֵ
			VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			RenderPassBeginInfo.clearValueCount = 1;
			RenderPassBeginInfo.pClearValues = &ClearColor;

			//������1.ָ������  2.��Ⱦ������Ϣ  3.����Ҫִ�е�ָ�����Ҫָ����У�û�и���ָ�����Ҫִ��
			vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			//��ͼ�ι���:�ڶ�����������ָ�����߶�����ͼ�ι��߻��Ǽ������
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			//�ύ���Ʋ�����ָ��壬
			//������1.ָ������ 2.���㻺�壬�����������һ�������� 3.1����ʾʵ����Ⱦ 4.������ɫ������gl_VertexIndex��ֵ 5.������ɫ������gl_InstanceIndex��ֵ
			vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

			//������Ⱦ����
			vkCmdEndRenderPass(m_CommandBuffers[i]);

			//������¼ָ�ָ���
			if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

		}



	}
};
