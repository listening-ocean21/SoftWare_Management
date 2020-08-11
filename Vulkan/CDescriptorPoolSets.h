#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define GLM_FORCE_RADIANS                //ȷ���� glm::rotate �����ĺ���ʹ�û�������Ϊ�������Ա����κο��ܵĻ���
#include <glm/gtc/matrix_transform.hpp>  //�����ṩ��������ģ�ͱ任����
#include <chrono>                        //�����ṩ��ʱ����

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <array>

//���幦��
//Ϊÿһ��vkBuffer�������������������uniform�������������а�,�����������ܱ�ֱ�Ӵ�������Ҫͨ���������ط���
namespace test9
{
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	//SDKͨ������VK_LAYER_LUNARG_standard_validaction�㣬
	//����ʽ�Ŀ��������������layers��
	//�Ӷ�������ȷ��ָ�����е���ȷ����ϲ㡣
	const std::vector<const char*> ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	//����������豸��չ�б�
	const std::vector<const char*> DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//����������ñ�����ָ��Ҫ���õ�layers�Լ��Ƿ�������
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	//��д��ṹ��VkDebugUtilsMessengerCreateInfoEXT ��Ϣ�󣬽�����Ϊһ����������vkCreateDebugUtilsMessengerEXT����������VkDebugUtilsMessengerEXT����
	//������vkCreateDebugUtilsMessengerEXT������һ����չ���������ᱻVulkan���Զ����أ���Ҫʹ��vkGetInstanceProcAddr����������
	//FUNCTION:һ������������Ҫ��Ϊ�˼���vkCreateDebugUtilsMessengerEXT����
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
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
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(vInstance, vCallback, pAllocator);
		}
	}


	//�����Vulkanʵ�ֿ��ܶԴ���ϵͳ������֧�֣����Ⲣ����ζ������ƽ̨��Vulkanʵ�ֶ�֧��ͬ��������
//��Ҫ��չisDeviceSuitable������ȷ���豸���������Ǵ����ı�������ʾͼ��
	struct EQueueFamilyIndices
	{	//֧�ֻ���ָ��Ķ��дغ�֧�ֱ��ֵĶ��д�
		int GraphicsFamily = -1;
		int PresentFamily = -1;

		bool isComplete()
		{
			return (GraphicsFamily >= 0 && PresentFamily >= 0);
		}
	};

	//��齻������ϸ�ڣ�1.surface����  2.surface��ʽ 3.���õĳ���ģʽ
	struct ESwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR Capablities;
		std::vector<VkSurfaceFormatKHR> SurfaceFormats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	//��������
	struct EVertex {
		glm::vec2 Pos;
		glm::vec3 Color;

		//һ�����ݱ��ύ��GPU���Դ��У�����Ҫ����Vulkan���ݵ�������ɫ�������ݵĸ�ʽ���������ṹ�����������ⲿ����Ϣ
		//1.�������ݰ�:
		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription BindingDescription = {};

			//��������Ŀ֮��ļ���ֽ����Լ��Ƿ�ÿ������֮�����ÿ��instance֮���ƶ�����һ����Ŀ
			BindingDescription.binding = 0;
			BindingDescription.stride = sizeof(EVertex);
			BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // �ƶ���ÿ����������һ��������Ŀ
			return BindingDescription;
		}

		//2.��δ����������
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> AttributeDescriptions = {};

			//һ�����������ṹ�����������˶���������δӶ�Ӧ�İ��������Ķ�����������������
			//|Pos����
			AttributeDescriptions[0].binding = 0;
			AttributeDescriptions[0].location = 0;    //��VertexShader���Ӧ��Pos����
			AttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  //32bit����������
			AttributeDescriptions[0].offset = offsetof(EVertex, Pos);

			//Color����
			AttributeDescriptions[1].binding = 0;
			AttributeDescriptions[1].location = 1; //Color����
			AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[1].offset = offsetof(EVertex, Color);

			return AttributeDescriptions;
		}
	};

	const std::vector<EVertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.5f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.5f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectiveMatrix;
	};

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
		//VkCommandBuffer m_CommandBuffer;
		//ָ���������ָ��ض������ʱ�Զ������������Ҫ�����Լ���ʽ�������
		std::vector<VkCommandBuffer> m_CommandBuffers;

		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;
		//���е��������󶨶��ᱻ�����һ��������VkDescriptorSetLayout����
		VkDescriptorSetLayout m_DescriptorSetLayout;

		//�ź���
		VkSemaphore ImageAvailableSemaphore;  //׼����Ⱦ���ź���
		VkSemaphore RenderFinishedSemaphore;  //��Ⱦ�����ź�����׼�����г���present

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;
		size_t m_CurrentFrame = 0;

		//�������������ڴ��ڴ�С�ı�󴥷�VK_ERROR_OUT_OF_DATE_KHR��Ϣ��
		//�����ִ��������ɿ�������������һЩ��������ʽ���ڴ��ڴ�С�ı�ʱ�ؽ���������
		//���һ���µı�������Ǵ��ڴ�С�Ƿ����ı䣺
		bool m_WindowResized = false;

		//���㻺��
		VkBuffer m_VertexBuffer;
		//�洢�ڴ���
		VkDeviceMemory m_VertexBufferMemory;

		//��������
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;

		//Uniform ������\
		//��ÿһ֡��������Ҫ�����µ����ݵ�UBO�����������Դ���һ���ݴ滺������û������ġ�
		//����������£���ֻ�����Ӷ���Ŀ��������ҿ��ܽ������ܶ������������ܡ���˱��β�ʹ����ʱ������
		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VkDeviceMemory> m_UniformBuffersMemory;

		//�������������ض���
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet>  m_DescriptorSets;

		//��������
		void __initWindow()
		{
			glfwInit();
			//����GLFW�������Ϊ��OpenGL��ƣ���Ҫ��ʽ����GLFW��ֹ���Զ�����OpenGL������
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
			//GLFW��������ʹ��glfwSetWindowUserPointer������ָ��洢�ڴ�������У�
			//��˿���ָ����̬���Ա����glfwGetWindowUserPointer����ԭʼ��ʵ������
			glfwSetWindowUserPointer(m_pWindow, this);
			//���ڴ��巢����С�仯��ʱ���¼��ص�
			glfwSetFramebufferSizeCallback(m_pWindow, CTest::__framebufferResizeCallback);
		}

		//�ص�����,����Ϊ��̬������������Ϊ�ص�����
		static void __framebufferResizeCallback(GLFWwindow* pWindow, int vWidth, int vHeight) {
			if (vWidth == 0 || vHeight == 0) return;

			auto app = reinterpret_cast<CTest*>(glfwGetWindowUserPointer(pWindow));
			app->m_WindowResized = true;
		}

		//����vulkanʵ��
		void __initVulkan()
		{
			__createInstance();
			//__setupDebugCallBack();
			__createSurface();
			//����ʵ��֮����Ҫ��ϵͳ���ҵ�һ��֧�ֹ��ܵ��Կ������ҵ�һ��ͼ����Ϊ�ʺ������豸
			__pickPhysicalDevice();
			__createLogicalDevice();
			__createSwapShain();
			__createImageViews();
			__createRenderPass();
			//Ҫ�ڹ��ߴ���ʱ��Ϊ��ɫ���ṩ����ÿ���������󶨵���ϸ��Ϣ,���ǵ����ڹ�����ʹ�ã������Ҫ�ڹ��ߴ���֮ǰ����
			__createDescritorSetLayout();
			__createGraphicsPipelines();
			__createFrameBuffers();
			__createCommandPool();
			__createVertexBuffer();
			__createIndexBuffer();
			__createUniformBuffer();
			//������������
			__createDescritorPool();
			__createDescriptorSet();
			__createCommandBuffers();
			__creatSemaphores(); //�����ź���
		}

		//�ؽ�������
		void  __recreateSwapChain()
		{
			int Width = 0, Height = 0;
			//��������С��ʱ�����ڵ�֡����ʵ�ʴ�СҲΪ0������Ӧ�ó����ڴ�����С����ֹͣ��Ⱦ��ֱ���������¿ɼ�ʱ�ؽ�������
			if (Width == 0 || Height == 0)
			{
				glfwGetFramebufferSize(m_pWindow, &Width, &Height);
				glfwWaitEvents();
			}
			vkDeviceWaitIdle(m_Device);

			__createSwapShain();
			__createImageViews();         //ͼ����ͼ�ǽ����ڽ�����ͼ������ϵ�
			__createRenderPass();         //�����ڽ�����ͼ��ĸ�ʽ
			__createGraphicsPipelines();  //����ͼ�ι����ڼ�ָ��Viewport��scissor ���δ�С,�ı���ͼ��Ĵ�С
			__createFrameBuffers();		  //�����ڽ�����ͼ��
			__createCommandBuffers();	  //�����ڽ�����ͼ��
		}

		//ѭ����Ⱦ
		void __mainLoop()
		{
			while (!glfwWindowShouldClose(m_pWindow))
			{
				glfwPollEvents();
				//���������еĲ��������첽�ġ���ζ�ŵ������˳�mainLoop��
				//���ƺͳ��ֲ���������Ȼ��ִ�С���������ò��ֵ���Դ�ǲ��Ѻõ�
				//Ϊ�˽��������⣬����Ӧ�����˳�mainLoop���ٴ���ǰ�ȴ��߼��豸�Ĳ������:
				__drawFrame();   //�첽����

				//����UniformBuffer
			}
			vkDeviceWaitIdle(m_Device);
		}

		//Ϊ��ȷ�����´�����صĶ���֮ǰ���ϰ汾�Ķ���ϵͳ��ȷ��������,��Ҫ�ع�Cleanup
		void __cleanUpSwapChain()
		{
			//ɾ��֡����
			for (size_t i = 0; i < m_SwapChainFrameBuffers.size(); i++)
			{
				vkDestroyFramebuffer(m_Device, m_SwapChainFrameBuffers[i], nullptr);
			}

			//ѡ�����vkFreeCommandBuffers���������Ѿ����ڵ�������������ַ�ʽ�������ö�������Ѿ���������������
			vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

			vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
			vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

			//ѭ��ɾ����ͼ
			for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
			{
				vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
			}

			vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		}

		//�˳�
		void __cleanUp()
		{
			__cleanUpSwapChain();

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
				vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
				vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
			}

			//ɾ��ָ��ض���
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

			//�������ڳ����˳�֮ǰΪ��Ⱦָ���ṩ֧�֣����������������������cleauo
			vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
			vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);

			vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
			vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);

			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

			//UBO�����ݽ����������еĻ���ʹ�ã����԰������Ļ�����ֻ����������٣�
			for (size_t i = 0; i < m_SwapChainImages.size(); i++)
			{
				vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
				vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);
				vkDestroyFence(m_Device, m_ImagesInFlight[i], nullptr);
			}
			;
			vkDestroyDevice(m_Device, nullptr);
			//ʹ��Vulkan API�����Ķ���Ҳ��Ҫ���������Ӧ����Vulkanʵ�����֮ǰ�������
			if (enableValidationLayers)
			{
				destroyDebugUtilsMessengerEXT(m_Instance, m_CallBack, nullptr);
			}
			//ע��ɾ��˳����ɾ��surface ��ɾ��Instance
			vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
			vkDestroyInstance(m_Instance, nullptr);

			glfwDestroyWindow(m_pWindow);
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
			//CreateInfo.enabledExtensionCount = 0;

			//DebugInfo
			VkDebugUtilsMessengerCreateInfoEXT  CreateDebugInfo;

			//�޸�VkInstanceCreateInfo�ṹ�壬��䵱ǰ�������Ѿ�������validation layers���Ƽ��ϡ�
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

			//1.3����Instance
			VkResult InstanceResult = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
			//����Ƿ񴴽��ɹ�
			if (InstanceResult != VK_SUCCESS) throw std::runtime_error("faild to create Instance!");

		}

		//ѡ��֧��������Ҫ�����Ե��豸
		void __pickPhysicalDevice()
		{
			//1.ʹ��VkPhysicalDevice���󴢴��Կ���Ϣ��������������Instance�������ʱ���Զ�����Լ������в����˹����
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
					m_PhysicalDevice = device;
					break;
				}
			}

			if (m_PhysicalDevice == VK_NULL_HANDLE)
			{
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}

		//�����߼��豸��
		//FUNC:��Ҫ�������豸���н���
		void __createLogicalDevice()
		{
			//��������
			EQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);

			//������������Ԥ������Ķ��и���,����֧�ֱ��ֺ���ʾ����
			std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
			//��Ҫ���VkDeviceQueueCreateInfo�ṹ��������ͬ���ܵĶ��С�һ����ʽ����Բ�ͬ���ܵĶ��дش���һ��set����ȷ�����дص�Ψһ��
			std::set<int> UniqueQueueFamilies = { Indices.GraphicsFamily, Indices.PresentFamily };

			//���ȼ�
			float QueuePriority = 1.0f;
			for (int queueFamily : UniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &QueuePriority;
				QueueCreateInfos.push_back(queueCreateInfo);
			}

			////ָ��ʹ�õ��豸����
			VkPhysicalDeviceFeatures PhysicalDeviceFeatures = {};

			////�����߼��豸
			VkDeviceCreateInfo DeviceCreateInfo = {};
			DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			////���ָ����д�����Ϣ�Ľṹ����豸���ܽṹ��:
			DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
			DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());

			DeviceCreateInfo.pEnabledFeatures = &PhysicalDeviceFeatures;

			DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
			DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

			////Ϊ�豸����validation layers
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

			//��ȡָ��������Ķ��о��
			vkGetDeviceQueue(m_Device, Indices.GraphicsFamily, 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, Indices.PresentFamily, 0, &m_PresentQueue);
		}
		//�ж��Ƿ�֧��gPU����
		bool __isDeviceSuitable(VkPhysicalDevice vDevice)
		{
			EQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);

			//�Ƿ�֧�ֽ�����֧��
			bool DeviceExtensionSupport = __checkDeviceExtensionSupport(vDevice);

			bool SwapChainAdequate = false;
			if (DeviceExtensionSupport)
			{
				ESwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
				//�����棬˵��������������������Ҫ
				SwapChainAdequate = !SwapChainSupport.SurfaceFormats.empty() && !SwapChainSupport.PresentModes.empty();
			}
			return (Indices.isComplete() && DeviceExtensionSupport && SwapChainAdequate);
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

		//���ûص�����
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
		ESwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice vPhsicalDevice)
		{
			ESwapChainSupportDetails Details;
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

		//���콻����
		void __createSwapShain()
		{
			//��ѯ����֧�ֽ�������
			ESwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(m_PhysicalDevice);
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
			EQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);
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
			SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;   //ָ��alphaͨ���Ƿ������ʹ���ϵͳ�е��������ڽ��л�ϲ���
			SwapChainCreateInfo.preTransform = SwapChainSupport.Capablities.currentTransform;
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
			m_SwapChainImageFormat = SurfaceFormat.format;
			m_SwapChainExtent = Extent;
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
			if (vkCreateShaderModule(m_Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule) != VK_SUCCESS)
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

			ColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
			VkRenderPassCreateInfo RenderPassCreateInfo = {};
			RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPassCreateInfo.attachmentCount = 1;
			RenderPassCreateInfo.pAttachments = &ColorAttachmentDescription;
			RenderPassCreateInfo.subpassCount = 1;
			RenderPassCreateInfo.pSubpasses = &SubpassDescription;

			//��ͨ����������Ⱦͨ���е���ͨ�����Զ������ֵı任����Щ�任ͨ����ͨ����������ϵ���п���
			VkSubpassDependency SubpassDependency = {};
			SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;  //ָ��Ҫ������ϵ
			SubpassDependency.dstSubpass = 0;  //������ͨ��������
			SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //ָ���ȴ��Ĳ���������ȴ���������ɶ�Ӧͼ��Ķ�ȡ�����������ͨ���ȴ���ɫ��������Ľ׶���ʵ��
			SubpassDependency.srcAccessMask = 0;
			//����ɫ�����׶εĲ������漰��ɫ�����Ķ�ȡ��д��Ĳ���Ӧ�õȴ�����Щ���ý���ֹת������
			SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			RenderPassCreateInfo.dependencyCount = 1;
			RenderPassCreateInfo.pDependencies = &SubpassDependency;

			if (vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		//����ͼ�ι���
		void __createGraphicsPipelines()
		{
			auto VertShaderCode = __readFile("vert.spv");
			auto FragShaderCode = __readFile("frag.spv");

			VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
			VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

			//��ɫ��������VkShaderModule����ֻ���ֽ��뻺������һ����װ��������ɫ����û�б˴�����
			//ͨ��VkPipelineShaderStageCreateInfo�ṹ����ɫ��ģ����䵽�����еĶ������Ƭ����ɫ���׶�

			//����
			VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
			VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			VertShaderStageInfo.module = VertShaderModule;
			VertShaderStageInfo.pName = "main";

			//Ƭ��
			VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
			FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			FragShaderStageInfo.module = FragShaderModule;
			FragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

			//������Ϣ
			auto BindingDescription = EVertex::getBindingDescription();
			auto AttributeDescription = EVertex::getAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo  PipelineVertexInputStateCreateInfo = {};
			PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &BindingDescription;
			PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t> (AttributeDescription.size());
			PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = AttributeDescription.data();

			//�������ݵļ���ͼԪ���˽ṹ���Ƿ����ö��������¿�ʼͼԪ
			VkPipelineInputAssemblyStateCreateInfo  PipelineInputAssemblyStateCreateInfo = {};
			PipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			PipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  //���㹲�棬���㲻����
			PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

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
			PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;                    //����ƬԪ�����������ߵĿ�ȡ������߿�֧��ȡ����Ӳ�����κδ���1.0���߿���Ҫ����GPU��wideLines����֧��
			PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;    //ָ������������ͷ�ʽ
			PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //front-facing��Ķ����˳�򣬿�����˳ʱ��Ҳ��������ʱ��
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

			//��Ҫ�ڴ������ߵ�ʱ��ָ�����������ϵĲ��֣����Ը�֪Vulkan��ɫ����Ҫʹ�õ��������������������ڹ��߲��ֶ�����ָ��
			VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
			PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutCreateInfo.setLayoutCount = 1; // Optional
			PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional


			if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			//��������
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
			GraphicsPipelineCreateInfo.layout = m_PipelineLayout;   //�Ǿ�����ǽṹ��
			GraphicsPipelineCreateInfo.renderPass = m_RenderPass;
			GraphicsPipelineCreateInfo.subpass = 0;
			//����ͨ���Ѿ����ڵĹ��ߴ����µ�ͼ�ι��ߣ�������ͬ����Լ�ڴ����ģ�
			GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			GraphicsPipelineCreateInfo.basePipelineIndex = -1;

			//����ͼ�ι���
			if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			//ɾ��
			vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
			vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
		}

		//����֡����
		void __createFrameBuffers()
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

		//����ָ���
		void __createCommandPool()
		{
			//ÿ��ָ��ض�������ָ������ֻ���ύ��һ���ض����͵Ķ��У�����ʹ�û���ָ��
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
				CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
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

				//�󶨶��㻺����
				VkBuffer VertexBuffers[] = { m_VertexBuffer };
				VkDeviceSize  OffSets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, VertexBuffers, OffSets);

				//����������
				vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

				//�����������ϰ󶨵�ʵ�ʵ���ɫ������������
				vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

				//�ύ���Ʋ�����ָ��壬
				//������1.ָ������������ 2.����instanceing����. 3��û��ʹ��instancing������ָ��1����������ʾ�����ݵ����㻺�����еĶ�������
				//4��ָ��������������ƫ������ʹ��1���ᵼ��ͼ�ο��ڵڶ�����������ʼ��ȡ  5.����������������ӵ�������ƫ�ơ� 6.ָ��instancingƫ����������û��ʹ�ø�����
				vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

				//������Ⱦ����
				vkCmdEndRenderPass(m_CommandBuffers[i]);

				//������¼ָ�ָ���
				if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to record command buffer!");
				}
			}
		}

		//���Ʋ��裺��Ҫͬ��������դ��������Ӧ�ó�������Ⱦ��������ͬ���������ź�������������������ڻ��߿��������ͬ��������ʵ��ͬ��
		//1.���ȣ��ӽ������л�ȡһͼ��
		//2.��֡������ʹ����Ϊ������ͼ��ִ����������е�����NDLE,
		//3.��Ⱦ���ͼ�񷵻ؽ�����
		//Ϊʲô��Ҫͬ���������������趼���첽����ģ�û��һ����˳��ִ�У�ʵ����ÿһ������Ҫ��һ���Ľ���������У������Ҫ�����ź�����ͬ��
		void __drawFrame()
		{
			//��ǰһ֡���
			vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

			uint32_t ImageIndex;
			//1.���ȣ��ӽ������л�ȡһͼ��
			VkResult Result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &ImageIndex);
			if (Result == VK_ERROR_OUT_OF_DATE_KHR)  //VK_ERROR_OUT_OF_DATE_KHR�����������ܼ���ʹ�á�ͨ�������ڴ��ڴ�С�ı��
			{
				__recreateSwapChain();
				return;
			}
			else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("failed to acquire SwapChain Image!");
			}

			__updateUniformBuffer(ImageIndex);

			//��CPU�ڵ�ǰλ�ñ���������Ȼ��һֱ�ȴ��������ܵ�Fence��Ϊsignaled��״̬
			//���鵱ǰ֡�Ƿ�ʹ�������ͼ��
			if (m_ImagesInFlight[ImageIndex] != VK_NULL_HANDLE) {
				vkWaitForFences(m_Device, 1, &m_ImagesInFlight[ImageIndex], VK_TRUE, UINT64_MAX);
			}
			//��۵�ǰ֡����ʹ�����ͼ��
			m_ImagesInFlight[ImageIndex] = m_InFlightFences[m_CurrentFrame];

			//2.�ύ����嵽ͼ�ζ���
			VkSubmitInfo SubmitInfo = {};
			SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			//���ź������һ��ͼ���Ѿ���ȡ����׼����Ⱦ������WaitStages��ʾ�������Ǹ��׶εȴ�
			VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
			VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			SubmitInfo.waitSemaphoreCount = 1;
			//ִ�п�ʼ֮ǰҪ�ȴ����ĸ��ź�����Ҫ�ȴ���ͨ�����ĸ��׶�
			SubmitInfo.pWaitSemaphores = WaitSemaphores;
			SubmitInfo.pWaitDstStageMask = WaitStages;
			//ָ���ĸ����������ʵ���ύִ�У�����Ϊ�ύ����������������Ǹջ�ȡ�Ľ�����ͼ����Ϊ��ɫ�������а󶨡�
			SubmitInfo.commandBufferCount = 1;
			SubmitInfo.pCommandBuffers = &m_CommandBuffers[ImageIndex];

			//ָ���˵��������ִ�н�������Щ�ź��������źš�������Ҫʹ��renderFinishedSemaphore����Ⱦ�Ѿ���ɿ��Գ�����
			VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
			SubmitInfo.signalSemaphoreCount = 1;
			SubmitInfo.pSignalSemaphores = SignalSemaphores;

			//������������
			vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

			//����������ύ��ͼ�ζ���
			//����������ִ�е�ʱ��Ӧ�ñ�ǵ�դ�������ǿ�������������һ��֡�Ѿ������
			if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer!");
			}

			//3.������ύ����������ʹ��������ʾ����Ļ��
			VkPresentInfoKHR PresentInfo = {};
			PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			PresentInfo.waitSemaphoreCount = 1;
			//ָ���ڽ���presentation֮ǰҪ�ȴ����ź���
			PresentInfo.pWaitSemaphores = SignalSemaphores;

			//ָ�������ύͼ��Ľ�������ÿ��������ͼ�����������������½�һ��
			VkSwapchainKHR SwapChains[] = { m_SwapChain };
			PresentInfo.swapchainCount = 1;
			PresentInfo.pSwapchains = SwapChains;
			PresentInfo.pImageIndices = &ImageIndex;
			//PresentInfo.pResults = nullptr;

			//�ύ�������ͼ���������
			Result = vkQueuePresentKHR(m_PresentQueue, &PresentInfo);
			if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_WindowResized) {
				m_WindowResized = false;
				__recreateSwapChain();
			}
			else if (Result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}

			//��ʼ������һ֮֡ǰ��ȷ�ĵȴ�presentation���:
			//�ó�����drawFrame�����п��ٵ��ύ����������ʵ����ȴ�������Щ�����Ƿ�����ˡ����CPU�ύ�Ĺ�����GPU�ܴ���Ŀ죬��ô���л����������������
			//���������1.����ʹ����drawFrame���ȴ��������
			//vkQueueWaitIdle(m_PresentQueue);
			//2.������󲢷�����֡����Ϊÿһ֡�����ź���
			//const int MAX_FRAMES_IN_FLIGHT = 2;
			//std::vector<VkSemaphore> imageAvailableSemaphores;
			//std::vector<VkSemaphore> renderFinishedSemaphores;

			//Ϊ��ÿ��ʹ����ȷ��Ե��ź���������Ҫ���ٵ�ǰ֡����������һ��֡������
			m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		//�����ź���
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

		//�������㻺��,������ɫ������֮��ʹ��CPU�ɼ��Ļ�����Ϊ��ʱ���壬ʹ���Կ���ȡ�Ͽ�Ļ�����Ϊ�����Ķ��㻺��
		//�µĹ���stagingBufferMemory��Ϊ�ڴ��stagingBuffer������������CPU���صĶ������ݡ�
		void __createVertexBuffer()
		{
			VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			__createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
				stagingBufferMemory);

			void* data;
			vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, vertices.data(), (size_t)bufferSize);
			vkUnmapMemory(m_Device, stagingBufferMemory);

			//���㻺����
			__createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |  //������Ա������ڴ洫�������Ŀ�Ļ���
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_VertexBuffer, m_VertexBufferMemory);

			//vertexBuffer���ڹ������ڴ����豸���еģ�����vkMapMemory���������������ڴ����ӳ�䡣
			//ֻ��ͨ��stagingBuffer����vertexBuffer��������
			__copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

			//���ʹ�ù��Ļ������͹������ڴ����
			vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
			vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
			//�󶨶��㻺��������__createCommandBuffer��������
		}

		//Ӧ���ݻ�������Ӧ�ó������ҵ���ȷ���ڴ�����
		uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vkMemoryPropertyFlags)
		{
			//������Ч���ڴ�����
			VkPhysicalDeviceMemoryProperties MemoryProperties;
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &MemoryProperties);

			for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++)
			{
				//��������Ҫ��������ÿ���ڴ����Ե����ͽ���AND�������ж��Ƿ�Ϊ1
				//memoryTypes���������˶��Լ�ÿ���ڴ����͵�������ԡ����Զ������ڴ�����⹦�ܣ����Դ�CPU����д�����ݡ�
				if (vTypeFilter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & vkMemoryPropertyFlags) == vkMemoryPropertyFlags) return i;
			}
			throw std::runtime_error("failed to find suitable memory type!");
		}

		//�������󻺳�
		void __createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage,
			VkMemoryPropertyFlags vProperties, VkBuffer& pBuffer, VkDeviceMemory& pBufferMemory)
		{
			VkBufferCreateInfo BufferCreateInfo = {};
			BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			BufferCreateInfo.size = vSize;
			BufferCreateInfo.usage = vUsage; //ָ��������������α�ʹ��
			BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; //����Ҳ�������ض��Ķ��д�ռ�л��߶��ͬʱ����,����������ʹ��ͼ�ζ��У�ʹ�ö�ռ����ģʽ

			if (vkCreateBuffer(m_Device, &BufferCreateInfo, nullptr, &pBuffer) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create buffer!");
			}

			//��Ȼ�������Ѿ������ã���ʵ���ϲ�û�з����κο����ڴ棬�����Ҫ�����ڴ�
			VkMemoryRequirements MemoryRequirements;
			vkGetBufferMemoryRequirements(m_Device, pBuffer, &MemoryRequirements);

			//�����ڴ�֮�󣬾ͷ����ڴ�
			VkMemoryAllocateInfo MemoryAllocateInfo = {};
			MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
			//�Կ����Է��䲻ͬ���͵��ڴ棬�������Ӧ���ݻ�������Ӧ�ó������ҵ���ȷ���ڴ�����
			MemoryAllocateInfo.memoryTypeIndex = __findMemoryType(MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			if (vkAllocateMemory(m_Device, &MemoryAllocateInfo, nullptr, &pBufferMemory) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate buffer memory!");
			}

			//������ڴ棬���ڴ������������,���ش����Ļ����������������ڴ����
			vkBindBufferMemory(m_Device, pBuffer, pBufferMemory, 0);
		}

		//���ƻ���������
		void __copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
		{
			VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
			CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			CommandBufferAllocateInfo.commandPool = m_CommandPool;
			CommandBufferAllocateInfo.commandBufferCount = 1;

			VkCommandBuffer CommandBuffer;
			vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, &CommandBuffer);

			//��¼�ڴ洫��ָ��
			VkCommandBufferBeginInfo  CommandBufferBeginInfo = {};
			CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;   //��Ǹ������������������ʹ�����ָ���

			vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);

			//�������ݸ���
			VkBufferCopy BufferCopy = {};    //ָ�����Ʋ���Դ����λ��ƫ��
			BufferCopy.srcOffset = 0;
			BufferCopy.dstOffset = 0;
			BufferCopy.size = vSize;
			vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &BufferCopy);

			//�˴��ڴ洫�����ʹ�õ�ָ���ֻ�и���ָ���¼�꣬�ͻ����ָ���Ļ�����ŷ�ܣ��ύָ�����ɴ������ִ��
			vkEndCommandBuffer(CommandBuffer);

			//�ȴ�����������,�����ֵȴ��ķ���
			//1.ʹ��դ��(fence)��ͨ��vkWaitForFences�����ȴ���ʹ��դ��(fence)����ͬ�������ͬ���ڴ洫�������������������Ż��ռ�Ҳ����
			//2.ͨ��vkQueueWaitIdle�����ȴ�
			VkSubmitInfo SubmitInfo = {};
			SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			SubmitInfo.commandBufferCount = 1;
			SubmitInfo.pCommandBuffers = &CommandBuffer;

			vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(m_GraphicsQueue);

			//���ָ������
			vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &CommandBuffer);
		}

		//��������
		void __createIndexBuffer()
		{
			VkDeviceSize BufferSize = sizeof(indices[0]) * indices.size();

			VkBuffer StagingBuffer;
			VkDeviceMemory StagingBufferMemory;
			__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				StagingBuffer, StagingBufferMemory);

			void* data;
			vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &data);
			memcpy(data, indices.data(), (size_t)BufferSize);
			vkUnmapMemory(m_Device, StagingBufferMemory);

			//���㻺����
			__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |  //������Ա������ڴ洫�������Ŀ�Ļ���
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_IndexBuffer, m_IndexBufferMemory);

			//vertexBuffer���ڹ������ڴ����豸���еģ�����vkMapMemory���������������ڴ����ӳ�䡣
			//ֻ��ͨ��stagingBuffer����vertexBuffer��������
			__copyBuffer(StagingBuffer, m_IndexBuffer, BufferSize);

			//���ʹ�ù��Ļ������͹������ڴ����
			vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
			vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
			//�󶨶��㻺��������__createCommandBuffer��������
		}

		//ÿ���󶨶���ͨ��VkDescriptorSetLayoutBinding�ṹ������
		void __createDescritorSetLayout()
		{
			VkDescriptorSetLayoutBinding  UBOLayoutBinding = {};
			UBOLayoutBinding.binding = 0;                                         //��ɫ����ʹ�õ�binding
			UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  //ָ����ɫ��������������
			UBOLayoutBinding.descriptorCount = 1;                                 //ָ����������ֵ������MVP�任Ϊ��UBO�������Ϊ1
			UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             //ָ������������ɫ���ĸ��׶α����ã�������ڶ�����ɫ����ʹ��������
			UBOLayoutBinding.pImmutableSamplers = nullptr;                        //��ͼ��������������й�

			//�ýṹ�����һ��������
			VkDescriptorSetLayoutCreateInfo UBOLayoutCreateInfo = {};
			UBOLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			UBOLayoutCreateInfo.bindingCount = 1;
			UBOLayoutCreateInfo.pBindings = &UBOLayoutBinding;

			if (vkCreateDescriptorSetLayout(m_Device, &UBOLayoutCreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout!");
			}
		}

		//uniform������
		//��ÿһ֡��������Ҫ�����µ����ݵ�UBO�����������Դ���һ���ݴ滺������û������ġ�����������£���ֻ�����Ӷ���Ŀ��������ҿ��ܽ������ܶ������������ܡ�
		void __createUniformBuffer()
		{
			VkDeviceSize BufferSize = sizeof(UniformBufferObject);
			m_UniformBuffers.resize(m_SwapChainImages.size());
			m_UniformBuffersMemory.resize(m_SwapChainImages.size());

			for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
				__createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					m_UniformBuffers[i], m_UniformBuffersMemory[i]);
			}
		}

		//����uniform����
		//�ú�������ÿһ֡�д����µı任������ȷ������ͼ����ת��������Ҫ�����µ�ͷ�ļ�ʹ�øù��ܣ�
		void __updateUniformBuffer(uint32_t vCurrentImage)
		{
			static auto StartTime = std::chrono::high_resolution_clock::now();

			auto CurrentTime = std::chrono::high_resolution_clock::now();
			float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

			UniformBufferObject UBO = {};
			UBO.ModelMatrix = glm::rotate(glm::mat4(1.0f), Time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			//glm::lookAt �������۾�λ�ã�����λ�ú��Ϸ���Ϊ������
			UBO.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			//ѡ��ʹ��FOVΪ45�ȵ�͸��ͶӰ�����������ǿ�߱ȣ����ü����Զ�ü��档��Ҫ����ʹ�õ�ǰ�Ľ�������չ�������߱ȣ��Ա��ڴ��������С��ο����µĴ����Ⱥ͸߶ȡ�
			UBO.ProjectiveMatrix = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);
			//GLM�����ΪOpenGL��Ƶģ����Ĳü������Y�Ƿ�ת�ġ��������������򵥵ķ�������ͶӰ������Y����������ӷ�ת
			UBO.ProjectiveMatrix[1][1] *= -1;

			//�Խ�UBO�е����ݸ��Ƶ�uniform������
			void* Data;
			vkMapMemory(m_Device, m_UniformBuffersMemory[vCurrentImage], 0, sizeof(UBO), 0, &Data);
			memcpy(Data, &UBO, sizeof(UBO));
			vkUnmapMemory(m_Device, m_UniformBuffersMemory[vCurrentImage]);
		}

		//������������
		void __createDescritorPool()
		{
			VkDescriptorPoolSize PoolSize;
			PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			PoolSize.descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

			//�����������صĴ�С
			VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
			DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			DescriptorPoolCreateInfo.pPoolSizes = &PoolSize;
			DescriptorPoolCreateInfo.poolSizeCount = 1;
			DescriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());  //ָ�����Է���������������������

			if (vkCreateDescriptorPool(m_Device, &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor pool!");
			}
		}

		//�и��������ؾͿ��Կ�ʼ������������������
		void __createDescriptorSet()
		{

			std::vector<VkDescriptorSetLayout> DescriptorSetLayouts(m_SwapChainImages.size(), m_DescriptorSetLayout);
			VkDescriptorSetAllocateInfo AllocateInfo = {};
			AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocateInfo.descriptorPool = m_DescriptorPool;
			AllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());
			AllocateInfo.pSetLayouts = DescriptorSetLayouts.data();

			m_DescriptorSets.resize(m_SwapChainImages.size());
			//����Ҫ��ȷ�������������ϣ���Ϊ���ǻ�����������������ٵ�ʱ���Զ���
			if (vkAllocateDescriptorSets(m_Device, &AllocateInfo, m_DescriptorSets.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor set!");
			}

			for (size_t i = 0; i < m_SwapChainImages.size(); i++)
			{
				//���������Ϸ����֮������������
				VkDescriptorBufferInfo BufferInfo{};
				BufferInfo.buffer = m_UniformBuffers[i];
				BufferInfo.offset = 0;
				BufferInfo.range = sizeof(UniformBufferObject);

				//����������������
				VkWriteDescriptorSet DescriptorWrite{};
				DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				DescriptorWrite.dstSet = m_DescriptorSets[i];
				DescriptorWrite.dstBinding = 0;         //Ϊ uniform buffer �󶨵������趨Ϊ0
				DescriptorWrite.dstArrayElement = 0;    //���������������飬������Ҫָ��Ҫ���µ�����������������û��ʹ�����飬���Լ򵥵�����Ϊ0
				DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; //ָ������������
				DescriptorWrite.descriptorCount = 1;		//ָ������������������Ҫ������
				DescriptorWrite.pBufferInfo = &BufferInfo;	//ָ�����������õĻ���������
				//DescriptorWrite.pImageInfo = nullptr;       //ָ�����������õ�ͼ������
				//DescriptorWrite.pTexelBufferView = nullptr; //ָ����������ͼ

				//����
				vkUpdateDescriptorSets(m_Device, 1, &DescriptorWrite, 0, nullptr);
				//������Ҫ����__createCommandBuffers������ʹ��cmdBindDescriptorSets�����������ϰ󶨵�ʵ�ʵ���ɫ�����������У�
			}
		}
	};
}