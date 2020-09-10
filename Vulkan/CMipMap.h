// LNK1107.cpp
// compile with: /clr /LD
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS                //ȷ���� glm::rotate �����ĺ���ʹ�û�������Ϊ�������Ա����κο��ܵĻ���
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE     //����GLM��������͸��ͶӰ����Ĭ��ʹ��OpenGL����ȷ�Χ�������� -1.0 �� 1.0��������Ҫʹ��GLM_FORCE_DEPTH_ZERO_TO_ONE���彫������Ϊʹ�� 0.0 �� 1.0 ��Vulkan��ȷ�Χ��
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>  //�����ṩ��������ģ�ͱ任����
#include <chrono>                        //�����ṩ��ʱ����

#define STB_IMAGE_IMPLEMENTATION
#include <STB_IMAGE/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

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
#include <unordered_map>    //������Ψһ�Ķ������Ӧ��������Ϣ��ɾ���ظ��Ķ�������

//���幦��
//1.ʵ������ӳ�䣬��ͼ���Ѿ�������ͼ������Ҫͼ�ι��߲�������ȡ����
//2.ͼ����ֱ�ӷ��ʣ���Ҫ����ͼ����ͼ��������ͼ��
//3.1ʹ������������ͼ����Դ���޸����������֣�����������غ����������ϣ��԰�������һ����ϵ�ͼ������������������֮�����ǻ����������ͼ���굽Vertex�����У�
//���޸�Ƭ����ɫ���������ж�ȡ��ɫ���������ڲ嶥����ɫ��	

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "./Images/viking_room.obj";
const std::string TEXTURE_PATH = "./Images/viking_room.png";

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

//��������
struct SVertex {
	glm::vec3 Pos;
	glm::vec3 Color;
	glm::vec2 TexCoord;

	//һ�����ݱ��ύ��GPU���Դ��У�����Ҫ����Vulkan���ݵ�������ɫ�������ݵĸ�ʽ���������ṹ�����������ⲿ����Ϣ
	//1.�������ݰ�:
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription BindingDescription = {};

		//��������Ŀ֮��ļ���ֽ����Լ��Ƿ�ÿ������֮�����ÿ��instance֮���ƶ�����һ����Ŀ
		BindingDescription.binding = 0;
		BindingDescription.stride = sizeof(SVertex);
		BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // �ƶ���ÿ����������һ��������Ŀ
		return BindingDescription;
	}

	//2.��δ����������
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions = {};

		//һ�����������ṹ�����������˶���������δӶ�Ӧ�İ��������Ķ�����������������
		//|Pos����
		AttributeDescriptions[0].binding = 0;
		AttributeDescriptions[0].location = 0;    //��VertexShader���Ӧ��Pos����
		AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  //32bit����������
		AttributeDescriptions[0].offset = offsetof(SVertex, Pos);

		//Color����
		AttributeDescriptions[1].binding = 0;
		AttributeDescriptions[1].location = 1; //Color����
		AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		AttributeDescriptions[1].offset = offsetof(SVertex, Color);

		//TexCoord����
		AttributeDescriptions[2].binding = 0;
		AttributeDescriptions[2].location = 2;
		AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		AttributeDescriptions[2].offset = offsetof(SVertex, TexCoord);
		return AttributeDescriptions;
	}

	bool operator==(const SVertex& other) const
	{
		return Pos == other.Pos && Color == other.Color && TexCoord == other.TexCoord;
	}
};

namespace std {
	template<> struct hash<SVertex> {
		size_t operator()(SVertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.Pos) ^
				(hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}
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

	//ͼ�������ر���Ϊ����Ԫ��
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	//����ͼ��
	VkImageView m_TextureImageView;
	//����������
	VkSampler  m_TextureSampler;

	//���ͼ����ȸ����ǻ���ͼ��ģ�������ɫ����������ͬ���ǽ����������Զ��������ͼ��
	//���ͼ����Ҫ������Դ��ͼ�� �ڴ棬 ͼ����ͼ
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	std::vector<SVertex> Vertices;
	std::vector<uint32_t> Indices;

	//mip����
	uint32_t m_MipLevels;

	//��������
	void __initWindow()
	{
		glfwInit();
		//����GLFW�������Ϊ��OpenGL��ƣ���Ҫ��ʽ����GLFW��ֹ���Զ�����OpenGL������
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		pWindow = glfwCreateWindow(WIDTH, HEIGHT, "VulKanTest", nullptr, nullptr);
		//GLFW��������ʹ��glfwSetWindowUserPointer������ָ��洢�ڴ�������У�
		//��˿���ָ����̬���Ա����glfwGetWindowUserPointer����ԭʼ��ʵ������
		glfwSetWindowUserPointer(pWindow, this);
		//���ڴ��巢����С�仯��ʱ���¼��ص�
		glfwSetFramebufferSizeCallback(pWindow, CTest::__framebufferResizeCallback);
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

		__createCommandPool();
		__createDepthResources();
		//ȷ�������ͼ����ͼʵ�ʴ��������֡����
		__createFrameBuffers();
		__createTextureImage();
		//����������Ҫ������ͼ���ܷ�������
		__createTextureImageView();
		//���ò����������Ժ��ʹ��������ɫ���ж�ȡ��ɫ������û���κεط�����VkImage����������һ�����صĶ������ṩ�˴���������ȡ��ɫ�Ľӿڡ�������Ӧ�����κ���������ͼ���У�������1D��2D��������3D
		__createTextureSampler();
		//��䶥�����������
		__loadModel();
		__creatSVertexBuffer();
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
			glfwGetFramebufferSize(pWindow, &Width, &Height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(m_Device);

		__createSwapShain();
		__createImageViews();         //ͼ����ͼ�ǽ����ڽ�����ͼ������ϵ�
		__createRenderPass();         //�����ڽ�����ͼ��ĸ�ʽ
		__createGraphicsPipelines();  //����ͼ�ι����ڼ�ָ��Viewport��scissor ���δ�С,�ı���ͼ��Ĵ�С
		__createDepthResources();     //�����Ⱦͨ���������ģ�渽���������ָ�����ģ��״̬��
		__createFrameBuffers();		  //�����ڽ�����ͼ��
		__createCommandBuffers();	  //�����ڽ�����ͼ��
	}

	//ѭ����Ⱦ
	void __mainLoop()
	{
		while (!glfwWindowShouldClose(pWindow))
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
		vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
		vkDestroyImage(m_Device, m_DepthImage, nullptr);
		vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

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

		vkDestroySampler(m_Device, m_TextureSampler, nullptr);
		vkDestroyImageView(m_Device, m_TextureImageView, nullptr);

		//UBO�����ݽ����������еĻ���ʹ�ã����԰������Ļ�����ֻ����������٣�
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);
			vkDestroyFence(m_Device, m_ImagesInFlight[i], nullptr);
		}

		vkDestroyImage(m_Device, m_TextureImage, nullptr);
		vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		//ʹ��Vulkan API�����Ķ���Ҳ��Ҫ���������Ӧ����Vulkanʵ�����֮ǰ�������
		if (enableValidationLayers)
		{
			destroyDebugUtilsMessengerEXT(m_Instance, m_CallBack, nullptr);
		}
		//ע��ɾ��˳����ɾ��surface ��ɾ��Instance
		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

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
		SQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);

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
		PhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

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
		SQueueFamilyIndices  Indices = __findQueueFamilies(vDevice);

		//�Ƿ�֧�ֽ�����֧��
		bool DeviceExtensionSupport = __checkDeviceExtensionSupport(vDevice);

		//���������˲���
		VkPhysicalDeviceFeatures SupportedFeatures;
		vkGetPhysicalDeviceFeatures(vDevice, &SupportedFeatures);

		bool SwapChainAdequate = false;
		if (DeviceExtensionSupport)
		{
			SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
			//�����棬˵��������������������Ҫ
			SwapChainAdequate = !SwapChainSupport.SurfaceFormats.empty() && !SwapChainSupport.PresentModes.empty();
		}
		return (Indices.isComplete() && DeviceExtensionSupport && SwapChainAdequate && SupportedFeatures.samplerAnisotropy);
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
		if (glfwCreateWindowSurface(m_Instance, pWindow, nullptr, &m_WindowSurface) != VK_SUCCESS) {
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
		int Width, Height;
		glfwGetWindowSize(pWindow, &Width, &Height);

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

	//��__createImageViews��_createTextureImagView����
	VkImageView __createImageView(VkImage vImage,uint32_t vMipLevels, VkFormat vFormat, VkImageAspectFlags vAspectFlags)
	{
		VkImageViewCreateInfo ImageViewCreateInfo = {};
		ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.image = vImage;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = vFormat;

		//components�ֶε�����ɫͨ��������ӳ���߼�
		ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//subresourceRangle�ֶ���������ͼ���ʹ��Ŀ����ʲô���Լ����Ա����ʵ���Ч����
		ImageViewCreateInfo.subresourceRange.aspectMask = vAspectFlags;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = vMipLevels;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView ImageView;
		if (vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &ImageView) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create ImageView!");
		}

		return ImageView;
	}

	//Ϊÿһ����������ͼ�񴴽�������ͼ��
	void __createImageViews()
	{
		//����ͼ�񼯴�С
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		//ѭ������������ͼ��
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			m_SwapChainImageViews[i] = __createImageView(m_SwapChainImages[i], 1, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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
		VkAttachmentDescription ColorAttachment = {}; //����ֻ��һ����ɫ���帽��
		ColorAttachment.format = m_SwapChainImageFormat;  //ָ����ɫ������ʽ�������뽻�����б���һ��
		ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;  //���ﲻ�����ز���
		//��Ⱦǰ����Ⱦ�������ڶ�Ӧ�����Ĳ�����Ϊ��Ӧ����ɫ���������
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  //��ʼ�׶���һ����������������
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //��Ⱦ�����ݻ�洢���ڴ棬����֮����ж�ȡ����
		//����ģ����
		ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //ͼ���ڽ������б�����
		ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//��ͨ����һ����������Ⱦͨ�������ɶ����ͨ����ɡ���ͨ������Ⱦ������һ������
		VkAttachmentReference ColorAttachmentRef = {};
		ColorAttachmentRef.attachment = 0;  //ͨ�����������������е�����������,����ֻ��һ����ɫ����
		ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//��ȸ���
		VkAttachmentDescription DepthAttachment = {};
		DepthAttachment.format = __findDepthFormat();
		DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference DepthAttachmentRef = {};
		DepthAttachmentRef.attachment = 1;
		DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//��ͨ��
		VkSubpassDescription SubpassDescription = {};
		SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //ָ��graphics subpassͼ����ͨ��
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pColorAttachments = &ColorAttachmentRef;  //ָ����ɫ�������á������������е�����ֱ�Ӵ�Ƭ����ɫ�����ã���layout(location = 0) out vec4 outColor ָ��!
		SubpassDescription.pDepthStencilAttachment = &DepthAttachmentRef;

		//��ͨ����������Ⱦͨ���е���ͨ�����Զ������ֵı任����Щ�任ͨ����ͨ����������ϵ���п���
		VkSubpassDependency SubpassDependency = {};
		SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;  //ָ��Ҫ������ϵ
		SubpassDependency.dstSubpass = 0;  //������ͨ��������
		SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //ָ���ȴ��Ĳ���������ȴ���������ɶ�Ӧͼ��Ķ�ȡ�����������ͨ���ȴ���ɫ��������Ľ׶���ʵ��
		SubpassDependency.srcAccessMask = 0;
		//����ɫ�����׶εĲ������漰��ɫ�����Ķ�ȡ��д��Ĳ���Ӧ�õȴ�����Щ���ý���ֹת������
		SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		//������Ⱦ�ܵ�
		std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
		VkRenderPassCreateInfo RenderPassCreateInfo = {};
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());;
		RenderPassCreateInfo.pAttachments = Attachments.data();
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.dependencyCount = 1;
		RenderPassCreateInfo.pDependencies = &SubpassDependency;

		if (vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	//����ͼ�ι���
	void __createGraphicsPipelines()
	{
		auto VertShaderCode = __readFile("./Shaders/vert.spv");
		auto FragShaderCode = __readFile("./Shaders/frag.spv");

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
		auto BindingDescription = SVertex::getBindingDescription();
		auto AttributeDescription = SVertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo  PipelinSVertexInputStateCreateInfo = {};
		PipelinSVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelinSVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		PipelinSVertexInputStateCreateInfo.pVertexBindingDescriptions = &BindingDescription;
		PipelinSVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t> (AttributeDescription.size());
		PipelinSVertexInputStateCreateInfo.pVertexAttributeDescriptions = AttributeDescription.data();

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

		//��Ȳ���
		VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
		DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencil.depthTestEnable = VK_TRUE;    //ָ���Ƿ�Ӧ�ý��µ���Ȼ���������Ȼ��������бȽϣ���ȷ���Ƿ�Ӧ�ñ�����
		DepthStencil.depthWriteEnable = VK_TRUE;   //ָ��ͨ����Ȳ��Ե��µ�Ƭ������Ƿ�Ӧ�ñ�ʵ��д����Ȼ�����
		DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  //ָ��ִ�б������߶���Ƭ�εıȽ�ϸ��
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;

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
		GraphicsPipelineCreateInfo.pVertexInputState = &PipelinSVertexInputStateCreateInfo;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencil;  //�����Ⱦͨ���������ģ�渽���������ָ�����ģ��״̬��
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
			std::array<VkImageView, 2> Attachments = { m_SwapChainImageViews[i], m_DepthImageView };

			VkFramebufferCreateInfo FrameBufferCreateInfo = {};
			FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FrameBufferCreateInfo.renderPass = m_RenderPass;
			FrameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());;
			FrameBufferCreateInfo.pAttachments = Attachments.data();
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
			std::array<VkClearValue, 2> ClearValues = {};
			ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			ClearValues[1].depthStencil = { 1.0f, 0 };
			RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
			RenderPassBeginInfo.pClearValues = ClearValues.data();

			//������1.ָ������  2.��Ⱦ������Ϣ  3.����Ҫִ�е�ָ�����Ҫָ����У�û�и���ָ�����Ҫִ��
			vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			//��ͼ�ι���:�ڶ�����������ָ�����߶�����ͼ�ι��߻��Ǽ������
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			//�󶨶��㻺����
			VkBuffer VertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize  OffSets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, VertexBuffers, OffSets);

			//����������
			vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

			//�����������ϰ󶨵�ʵ�ʵ���ɫ������������
			vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

			//�ύ���Ʋ�����ָ��壬
			//������1.ָ������������ 2.����instanceing����. 3��û��ʹ��instancing������ָ��1����������ʾ�����ݵ����㻺�����еĶ�������
			//4��ָ��������������ƫ������ʹ��1���ᵼ��ͼ�ο��ڵڶ�����������ʼ��ȡ  5.����������������ӵ�������ƫ�ơ� 6.ָ��instancingƫ����������û��ʹ�ø�����
			vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(Indices.size()), 1, 0, 0, 0);

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
	void __creatSVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(Vertices[0]) * Vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		__createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, Vertices.data(), (size_t)bufferSize);
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
		VkCommandBuffer CommandBuffer;

		CommandBuffer = __beginSingleTimeCommands();

		//�������ݸ���
		VkBufferCopy BufferCopy = {};    //ָ�����Ʋ���Դ����λ��ƫ��
		BufferCopy.srcOffset = 0;
		BufferCopy.dstOffset = 0;
		BufferCopy.size = vSize;
		vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &BufferCopy);

		__endSingleTimeCommands(CommandBuffer);
	}

	//���ֱ任
	VkCommandBuffer __beginSingleTimeCommands()
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
		return CommandBuffer;
	}

	//���ֱ任
	void  __endSingleTimeCommands(VkCommandBuffer vCommandBuffer)
	{
		//�˴��ڴ洫�����ʹ�õ�ָ���ֻ�и���ָ���¼�꣬�ͻ����ָ���Ļ�����ŷ�ܣ��ύָ�����ɴ������ִ��
		vkEndCommandBuffer(vCommandBuffer);

		//�ȴ�����������,�����ֵȴ��ķ���
		//1.ʹ��դ��(fence)��ͨ��vkWaitForFences�����ȴ���ʹ��դ��(fence)����ͬ�������ͬ���ڴ洫�������������������Ż��ռ�Ҳ����
		//2.ͨ��vkQueueWaitIdle�����ȴ�
		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &vCommandBuffer;

		vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);

		//���ָ������
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &vCommandBuffer);

	}

	//ʹ�û�������ȡͼ��������Ҫ��ͼ������ȷ�Ĳ�����
	void __transitionImageLayout(VkImage vImage,uint16_t vMipLevels, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout VNewLayout)
	{
		VkCommandBuffer CommandBuffer = __beginSingleTimeCommands();

		//�������������ڴ���ͼ��任��ʹ�� image memory barrier,��Ҫ���ڷ�����Դ��ͬ������
		VkImageMemoryBarrier  ImageMemoryBarrier = {};
		ImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarrier.oldLayout = vOldLayout;
		ImageMemoryBarrier.newLayout = VNewLayout;
		//�����Դ�����дص�����ʹ�����ϣ�������������Ҫ���ö��дص���������������ģ����������VK_QUEUE_FAMILY_IGNORED(����Ĭ��ֵ)��
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		//image��subresourceRangeָ���ܵ�Ӱ���ͼ���ͼ����ض�����
		//���ǵ�ͼ�������飬Ҳû��ʹ��mipmapping levels������ָֻ��һ��������һ���㡣
		ImageMemoryBarrier.image = vImage;
		ImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		ImageMemoryBarrier.subresourceRange.levelCount = vMipLevels;
		ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarrier.subresourceRange.layerCount = 1;

		ImageMemoryBarrier.srcAccessMask = 0;
		ImageMemoryBarrier.dstAccessMask = 0;

		//Ԥ����:ɫ����ȡ����Ӧ�õȴ�����д�룬�ر��� fragment shader���ж�ȡ����Ϊ����Ҫʹ������ĵط�
		VkPipelineStageFlags SourceStage;
		VkPipelineStageFlags DestinationStage;

		//�����ȷ�ķ�������͹��߽׶Σ�
		if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && VNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			ImageMemoryBarrier.srcAccessMask = 0;
			ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && VNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && VNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			ImageMemoryBarrier.srcAccessMask = 0;
			ImageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}


		if (VNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			ImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (__hasStencilComponent(vFormat))
			{
				ImageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			ImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		//������Ҫ����ͬ��Ŀ�ģ����Ա�����Ӧ������ǰָ����һ�ֲ������ͼ��漰������Դ��ͬʱҪָ����һ�ֲ�������Դ����ȴ����ϡ�
		vkCmdPipelineBarrier(
			CommandBuffer,
			SourceStage, DestinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageMemoryBarrier
		);

		__endSingleTimeCommands(CommandBuffer);
	}

	//������������ͼ��
	void __copyBufferToImage(VkBuffer vBuffer, VkImage vImage, uint32_t vWidth, uint32_t vHeight)
	{
		VkCommandBuffer CommandBuffer = __beginSingleTimeCommands();

		//ָ������������һ���ֵ�ͼ�������
		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;       //ָ����������byteƫ����
		Region.bufferRowLength = 0;    //ָ���������ڴ��еĲ��֣�0��ʾ���ؽ�������
		Region.bufferImageHeight = 0;

		//imageSubresource��imageOffset �� imageExtent�ֶ�ָ�����ǽ�Ҫ����ͼ�����һ��������
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;

		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = {
			vWidth,
			vHeight,
			1
		};

		//������������ͼ��Ĳ�������ʹ��vkCmdCopyBufferToImage�����������У�
		vkCmdCopyBufferToImage(
			CommandBuffer,
			vBuffer,
			vImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&Region
		);

		__endSingleTimeCommands(CommandBuffer);
	}

	//��������
	void __createIndexBuffer()
	{
		VkDeviceSize BufferSize = sizeof(Indices[0]) * Indices.size();

		VkBuffer StagingBuffer;
		VkDeviceMemory StagingBufferMemory;
		__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			StagingBuffer, StagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &data);
		memcpy(data, Indices.data(), (size_t)BufferSize);
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
		//UBO������
		VkDescriptorSetLayoutBinding  UBOLayoutBinding = {};
		UBOLayoutBinding.binding = 0;                                         //��ɫ����ʹ�õ�binding
		UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  //ָ����ɫ��������������
		UBOLayoutBinding.descriptorCount = 1;                                 //ָ����������ֵ������MVP�任Ϊ��UBO�������Ϊ1
		UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             //ָ������������ɫ���ĸ��׶α����ã�������ڶ�����ɫ����ʹ��������
		UBOLayoutBinding.pImmutableSamplers = nullptr;                        //��ͼ��������������й�

		//��һ�������������ͼ��ȡ����(combined image sampler)
		VkDescriptorSetLayoutBinding SamplerLayoutBinding{};
		SamplerLayoutBinding.binding = 1;
		SamplerLayoutBinding.descriptorCount = 1;
		SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SamplerLayoutBinding.pImmutableSamplers = nullptr;
		//ָ����Ƭ����ɫ����ʹ�����ͼ�������������
		SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { UBOLayoutBinding, SamplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
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
		std::array<VkDescriptorPoolSize, 2> PoolSizes = {};
		PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		PoolSizes[0].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

		PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		PoolSizes[1].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

		//�����������صĴ�С
		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
		DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescriptorPoolCreateInfo.pPoolSizes = PoolSizes.data();
		DescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
		DescriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());  //ָ�����Է���������������������

		if (vkCreateDescriptorPool(m_Device, &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	//�и��������ؾͿ��Կ�ʼ������������������
	// ���һ���ǽ�ʵ�ʵ�ͼ��Ͳ�������Դ�󶨵������������еľ���������
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

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_TextureImageView;
			imageInfo.sampler = m_TextureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_DescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &BufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_DescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;       ////Ϊ uniform buffer �󶨵������趨Ϊ0
			descriptorWrites[1].dstArrayElement = 0;  ////���������������飬������Ҫָ��Ҫ���µ�����������������û��ʹ�����飬���Լ򵥵�����Ϊ0
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //ָ������������
			descriptorWrites[1].descriptorCount = 1;     //ָ������������������Ҫ������
			descriptorWrites[1].pImageInfo = &imageInfo; //ָ�����������õ�ͼ������

			vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			////������Ҫ����__createCommandBuffers������ʹ��cmdBindDescriptorSets�����������ϰ󶨵�ʵ�ʵ���ɫ�����������У�
		}
	}

	//����ͼƬ���ύ��VulKanͼ������У�;�л��õ���������������__createCommandPool����֮��
	void __createTextureImage()
	{
		int TexWidth, TexHeight, TexChannels;

		//STBI_rgb_alphaֵǿ�Ƽ���ͼƬ��alphaͨ������ʹû�и�ͨ��ֵ�������������У�ÿ���������ĸ��ֽ�
		stbi_uc* Pixels = stbi_load(TEXTURE_PATH.c_str(), &TexWidth, &TexHeight, &TexChannels, STBI_rgb_alpha);
		VkDeviceSize ImageSize = TexWidth * TexHeight * 4;

		if (!Pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		//���㼶��
		m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(TexWidth, TexHeight)))) + 1;

		VkBuffer StagingBuffer;
		VkDeviceMemory StagingBufferMemory;

		//������ʱ������
		__createBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			StagingBuffer, StagingBufferMemory);
		void* data;
		vkMapMemory(m_Device, StagingBufferMemory, 0, ImageSize, 0, &data);
		memcpy(data, Pixels, static_cast<size_t>(ImageSize));
		vkUnmapMemory(m_Device, StagingBufferMemory);

		//��Ҫ��������ԭͼ�����������
		stbi_image_free(Pixels);
		__createImage(TexWidth, TexHeight,m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_TextureImage, m_TextureImageMemory);

		//������ͼͼ����һ��copy�ݴ滺��������ͼͼ��
		//1.�任��ͼͼ�� VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		__transitionImageLayout(m_TextureImage, m_MipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//2.ִ�л�������ͼ��Ŀ�������
		__copyBufferToImage(StagingBuffer, m_TextureImage, static_cast<uint32_t>(TexWidth), static_cast<uint32_t>(TexHeight));
		//��shader��ɫ���п�ʼ����ͼͼ��Ĳ�����������Ҫ���һ���任��׼����ɫ������
		//__transitionImageLayout(m_TextureImage, m_MipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		__generateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, TexWidth, TexHeight, m_MipLevels);

		vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
		vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
	}

	//����ͼ�������ڴ����
	void __createImage(uint32_t vWidth, uint32_t vHeight, uint32_t vMipLevels,VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage,
		VkMemoryPropertyFlags vProperties, VkImage& vImage, VkDeviceMemory& vImageMemory)
	{
		//������ͨ����ɫ�����ʻ�����������ֵ������VuklKam�����ʹ��image������ʻ����������Կ��ټ�����ɫ
		VkImageCreateInfo ImageCreateInfo = {};
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                   //ָ��ͼ�����ͣ���֪Vulkan����ʲô��������ϵ��ͼ���вɼ����ء���������1D��2D��3Dͼ��1Dͼ�����ڴ洢�������ݻ��߻Ҷ�ͼ��2Dͼ����Ҫ����������ͼ��3Dͼ�����ڴ洢�������ء�
		ImageCreateInfo.extent.width = static_cast<uint32_t>(vWidth); //ͼ��ߴ�
		ImageCreateInfo.extent.height = static_cast<uint32_t>(vHeight);
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = vMipLevels;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.format = vFormat;         //ָ��ͼ���ʽ�������뱣֤����������������һ���ĸ�ʽ������copyʧ��
		ImageCreateInfo.tiling = vTiling;          //���ػ��ھ����ʵ�������岼�֣���ʵ����ѷ���,���У��������Ժ��޸ģ������Ҫ���ڴ�ͼ���и�ֱ�ӷ������أ�����ʹ��VK_IMAGE_TILING_LINEAR�����ػ���������Ĳ��֣���pixels���飩
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //GPU����ʹ�ã���һ���任���������أ������Ｘ�����б�Ҫ�ڵ�һ��ת��ʱ��������
		ImageCreateInfo.usage = vUsage; //ͼ�񽫻ᱻ����������copyĿ�꣬����ͼ����Ϊ����Ŀ�ĵأ�����ϣ������ɫ���з���ͼ���mesh������ɫ
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;   //ͼ�����һ�����д���ʹ�ã�֧��ͼ�λ��ߴ������
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;           //��־λ����ز�����ء��������������Ϊ������ͼ���������Ǽ��һ��������ֵ����ϡ��ͼ����ص�ͼ����һЩ��ѡ�ı�־
		ImageCreateInfo.flags = 0;

		if (vkCreateImage(m_Device, &ImageCreateInfo, nullptr, &vImage) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		//Ϊͼ���������ڴ�
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(m_Device, vImage, &MemoryRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = MemoryRequirements.size;
		allocInfo.memoryTypeIndex = __findMemoryType(MemoryRequirements.memoryTypeBits, vProperties);

		if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &vImageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(m_Device, vImage, vImageMemory, 0);
	}

	//����������ͼ
	void __createTextureImageView()
	{
		m_TextureImageView = __createImageView(m_TextureImage, m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	//��ɫ��ֱ�Ӵ�ͼ���ж�ȡ�����ǿ��Եģ����ǵ�������Ϊ����ͼ���ʱ�򲢲�����������ͼ��ͨ��ʹ�ò���������
	//��������������
	void __createTextureSampler()
	{
		VkSamplerCreateInfo SamplerInfo = {};
		//ָ���������ͱ任
		SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerInfo.magFilter = VK_FILTER_LINEAR;   //ָ�����طŴ��ڲ�ֵ��ʽ
		SamplerInfo.minFilter = VK_FILTER_LINEAR;   //ָ��������С�ڲ�ֵ��ʽ

		//ָ��ÿ������ʹ�õ�Ѱַģʽ������ռ�����UVW��Ӧ��XYZ
		SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  //������ͼ��ߴ��ʱ�����ѭ����䡣
		SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		SamplerInfo.anisotropyEnable = VK_TRUE; // ָ���Ƿ�ʹ�ø������Թ�������û�����ɲ�ʹ�ø����ԣ�����������һ������
		SamplerInfo.maxAnisotropy = 16;         //���ƿ����ڼ���������ɫ�����ز������������͵���ֵ��õ��ȽϺõ����ܣ����ǻ�õ��ϲ������

		//��������Χ����ͼ��ʱ�򷵻ص���ɫ����֮��Ӧ���Ǳ�ԵѰַģʽ��������float����int��ʽ���غ�ɫ����ɫ����͸���ȡ����ǲ���ָ��������ɫ
		SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		//ָ��ʹ�õ�����ϵͳ�����ڷ���ͼ������ء�����ֶ�ΪVK_TRUE����ζ�ſ��Լ򵥵�ʹ�����귶ΧΪ [ 0, texWidth ) �� [ 0, texHeight )��
		//���ʹ��VK_FALSE����ζ��ÿ���������ط���ʹ�� [ 0, 1) ��Χ����ʵ��Ӧ�ó�������ʹ�ù�һ�������ꡣ
		SamplerInfo.unnormalizedCoordinates = VK_FALSE;

		//��������ȽϹ��ܣ���ô�������Ⱥ�ֵ���бȽϣ����ұȽϺ��ֵ���ڹ��˲�������Ҫ������Ӱ����ӳ���percentage-closer filtering ���ٷֱȽ��ƹ�������
		SamplerInfo.compareEnable = VK_FALSE;
		SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		//mipmapping
		SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		SamplerInfo.mipLodBias = 0.0f;
		SamplerInfo.minLod = 0;
		SamplerInfo.maxLod = static_cast<float>(m_MipLevels);

		if (vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	//s�������ͼ�����Դ
	void __createDepthResources()
	{
		//��ȸ�ʽ
		VkFormat DepthFormat = __findDepthFormat();

		__createImage(m_SwapChainExtent.width, m_SwapChainExtent.height,1, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
		m_DepthImageView = __createImageView(m_DepthImage,1, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		//�任Ϊ���ʵ���ȸ���ʹ�õĲ��֣�����������ѡ��ʹ�ù������ϣ���Ϊ�任ֻ�ᷢ��һ��
		//__transitionImageLayout(m_DepthImage, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


	}
	//������������ĸ�ʽ����������֧��ʹ����ȸ�����
	VkFormat __findDepthFormat() {
		return __findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	//����һ����֧�ֵĸ�ʽ
	VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
	{
		for (VkFormat format : vCandidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (vTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & vFeatures) == vFeatures) {
				return format;
			}
			else if (vTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & vFeatures) == vFeatures) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	//��ѡ�����ȸ�ʽ�Ƿ����ģ�����
	bool __hasStencilComponent(VkFormat vFormat) {
		return vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	//����ģ��
	void __loadModel() {
		tinyobj::attrib_t Attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&Attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<SVertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				SVertex vertex{};

				vertex.Pos = {
					Attrib.vertices[3 * index.vertex_index + 0],
					Attrib.vertices[3 * index.vertex_index + 1],
					Attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.TexCoord = {
					Attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - Attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.Color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.count(vertex) == 0) 
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
					Vertices.push_back(vertex);
				}

				Indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	//����mipmap
	void __generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = __beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		__endSingleTimeCommands(commandBuffer);
	}
};
	