#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define GLM_FORCE_RADIANS                //ȷ���� glm::rotate �����ĺ���ʹ�û�������Ϊ�������Ա����κο��ܵĻ���
#include <glm/gtc/matrix_transform.hpp>  //�����ṩ��������ģ�ͱ任����
#include <chrono>                        //�����ṩ��ʱ����

#define STB_IMAGE_IMPLEMENTATION
#include <STB_IMAGE/stb_image.h>

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

#include "../Camera/CCamera.h"

#define SHADOWMAP_SIZE  1024

//���幦��
//ʵ����Ӱ��ͼ
namespace test12
{
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;


	//����ƶ�λ�ü�¼
	float LastX = WIDTH / 2, LastY = HEIGHT / 2;
	float ZNear = 1.0f, ZFar = 96.0f;
	//������Ұ
	float Fov = 45.0f;
	bool FirstMouse = true;

	float AmbientStrenght = 0.1f;
	float SpecularStrenght = 0.8f;

	glm::vec3 BaseLight = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 LightPos = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 LightDirection = glm::vec3(-1.0f, -1.0f, -1.0f);
	glm::vec3 FlashColor = glm::vec3(1.0f, 1.0f, 1.0f);

	CCamera Camera(glm::vec3(0.0f, 2.0f, 8.0f), glm::radians(-15.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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
		glm::vec3 Normal;
		glm::vec2 TexCoord;

		//������Ϣ
		glm::vec3 Ambient;  //������
		glm::vec3 Diffuse;  //������
		glm::vec3 Specular; //���淴��
		float Shininess;


		//һ�����ݱ��ύ��GPU���Դ��У�����Ҫ����Vulkan���ݵ�������ɫ�������ݵĸ�ʽ���������ṹ�����������ⲿ����Ϣ
		//1.�������ݰ�:
		static std::vector<VkVertexInputBindingDescription> getBindingDescription()
		{
			std::vector<VkVertexInputBindingDescription> BindingDescription = {};
			BindingDescription.resize(1);
			//��������Ŀ֮��ļ���ֽ����Լ��Ƿ�ÿ������֮�����ÿ��instance֮���ƶ�����һ����Ŀ
			BindingDescription[0].binding = 0;
			BindingDescription[0].stride = sizeof(SVertex);
			BindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // �ƶ���ÿ����������һ��������Ŀ
			return BindingDescription;
		}

		//2.��δ����������
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
		{
			std::vector<VkVertexInputAttributeDescription> AttributeDescriptions = {};
			AttributeDescriptions.resize(8);
			//һ�����������ṹ�����������˶���������δӶ�Ӧ�İ��������Ķ�����������������
			//|Pos����
			AttributeDescriptions[0].binding = 0;
			AttributeDescriptions[0].location = 0;    //��VertexShader���Ӧ��Pos����
			AttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  //32bit����������
			AttributeDescriptions[0].offset = offsetof(SVertex, Pos);

			//Color����
			AttributeDescriptions[1].binding = 0;
			AttributeDescriptions[1].location = 1; //Color����
			AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[1].offset = offsetof(SVertex, Color);

			//����������
			AttributeDescriptions[2].binding = 0;
			AttributeDescriptions[2].location = 2;
			AttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[2].offset = offsetof(SVertex, Normal);

			//TexCoord����
			AttributeDescriptions[3].binding = 0;
			AttributeDescriptions[3].location = 3;
			AttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			AttributeDescriptions[3].offset = offsetof(SVertex, TexCoord);

			//��������
			AttributeDescriptions[4].binding = 0;
			AttributeDescriptions[4].location = 4;
			AttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[4].offset = offsetof(SVertex, Ambient);

			AttributeDescriptions[5].binding = 0;
			AttributeDescriptions[5].location = 5;
			AttributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[5].offset = offsetof(SVertex, Diffuse);

			AttributeDescriptions[6].binding = 0;
			AttributeDescriptions[6].location = 6;
			AttributeDescriptions[6].format = VK_FORMAT_R32G32B32_SFLOAT;
			AttributeDescriptions[6].offset = offsetof(SVertex, Specular);

			AttributeDescriptions[7].binding = 0;
			AttributeDescriptions[7].location = 7;
			AttributeDescriptions[7].format = VK_FORMAT_R32_SFLOAT;
			AttributeDescriptions[7].offset = offsetof(SVertex, Shininess);

			return AttributeDescriptions;
		}

		bool operator==(const SVertex vVertex)
		{
			return Pos == vVertex.Pos && Color == vVertex.Color && TexCoord == vVertex.TexCoord;
		}
	};

	//����������Ͻ� 0��0 �����½ǵ� 1��1 ��ӳ������
	const std::vector<SVertex> vertices = {
		//positions             colors             normals             texture coords m.ambient            m.diffuse           m.specular        m.shininess
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f, -1.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  0.0f,  1.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{-1.0f, 0.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{1.0f,  0.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f, -1.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{1.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{1.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f},{0.0f,  1.0f,  0.0f},{0.0f,  1.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},

		{{ 10.0f, -1.5f,  10.0f}, {1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{ 10.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-10.0f, -1.5f,  10.0f}, {1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{  0.0f,  0.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{-10.0f, -1.5f, -10.0f}, {1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{  0.0f, 10.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f},
		{{ 10.0f, -1.5f, -10.0f}, {1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{ 10.0f, 10.0f},{1.0f, 0.5f, 0.31f}, {1.0f, 0.5f, 0.31f},{0.5f, 0.5f, 0.5f},32.0f}
	};

	const std::vector<uint16_t> indices = {
		   2, 1, 0, 5, 4, 3,
		   6, 7, 8, 9, 10,11,
		   12,13,14,15,16,17,
		   20,19,18,23,22,21,
		   24,25,26,27,28,29,
		   32,31,30,35,34,33,
		   38,37,36,36,39,38,
	};

	struct UniformBufferObject {
		glm::mat4 ModelMatrix;
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectiveMatrix;
		glm::mat4 LightSpaceMatrix; //��Դת������ ����������ϵת���ü�����ϵ

		glm::vec3 BaseLight;
		glm::vec3 LightPos;
		glm::vec3 LightDirection;
		glm::vec3 ViewPos;

		float AmbientStrenght;
		float SpecularStrenght;

		glm::vec3 FlashColor;
		glm::vec3 FlashPos;
		glm::vec3 FlashDir;
		float OuterCutOff;
		float InnerCutOff;

		float foo1 = 0.0f, foo2 = 0.0f, foo3 = 0.0f;
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

		//VkPipelineLayout m_PipelineLayout;
		//VkPipeline m_GraphicsPipeline;
		VkPipelineCache m_PipelineCache;
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
		//std::vector<VkBuffer> m_UniformBuffers;
		//std::vector<VkDeviceMemory> m_UniformBuffersMemory;

		//�������������ض���
		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSet  m_DescriptorSets;

		//ͼ�������ر���Ϊ����Ԫ��
		VkImage m_TextureImage;
		VkDeviceMemory m_TextureImageMemory;
		//����ͼ��
		VkImageView m_TextureImageView;
		//����������
		VkSampler  m_TextureSampler;

		//���ģ�����ʱ
		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		VkFormat depthFormat;

		VkSubmitInfo SubmitInfo;
		/** @brief Pipeline stages used to wait at for graphics queue submissions */
		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		struct {
			VkBuffer scene;
			VkBuffer offscreen;
		} uniformBuffers;

		struct {
			VkDeviceMemory scene;
			VkDeviceMemory offscreen;
		} uniformBufferMemorys;

		//Ϊ���������ͼ׼��
		struct LightSpaceMatrix {
			glm::mat4 LightSpaceVP; //��Դת������ ����������ϵת���ü�����ϵ
			glm::mat4 ModelMatrix;
		}LSM;

		//Ϊ�����ͼ��Ⱦ�ĸ�������
		struct SFrameBufferAttachment {
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};
		struct {
			VkPipelineLayout offscreen;
			VkPipelineLayout m_PipelineLayout;
		} pipelineLayouts;

		struct {
			VkDescriptorSet  offscreen;
			VkDescriptorSet  scene;
		} descriptorSets;

		//����pipeline:������Ⱦ����  ������� ������Ӱ��ͼ�������������
		struct {
			VkPipeline offscreen;
			VkPipeline graphicsPipeline;
			//VkPipeline sceneShadowPCF;
		} pipelines;

		struct {
			// Swap chain image presentation
			VkSemaphore presentComplete;
			// Command buffer submission and execution
			VkSemaphore renderComplete;
		} semaphores;
		//�����ͼ
		struct SDepthMapPass {
			int32_t width, height;
			VkFramebuffer frameBuffer;
			SFrameBufferAttachment depth;
			VkRenderPass renderPass;
			VkSampler depthSampler;
			VkDescriptorImageInfo descriptor;
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VkSemaphore semaphore = VK_NULL_HANDLE;
		} DepthMapPass;


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

			glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL); //�����������
			//ע��
			glfwSetCursorPosCallback(pWindow, __mouse_callback);
			glfwSetCursorPosCallback(pWindow, __scroll_callback);
		}

		//���ص�����
		static void __mouse_callback(GLFWwindow* pWindow, double xPos, double yPos)
		{
			if (FirstMouse)
			{
				LastX = xPos;
				LastY = yPos;
				FirstMouse = false;
			}

			double DeltaX, DeltaY;
			DeltaX = xPos - LastX;
			DeltaY = yPos - LastY;
			LastX = xPos;
			LastY = yPos;

			Camera.processMouseMovement(DeltaX, DeltaY);
		}

		//����������
		static void __scroll_callback(GLFWwindow* pWindow, double xPos, double yPos)
		{
			if (Fov >= 1.0f || Fov <= 45.0)
			{
				Fov -= yPos;
			}
			else if (Fov < 1.0f)
			{
				Fov = 1.0f;
			}
			else if (Fov > 45.0f)
			{
				Fov = 45.0f;
			}
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
			assert(__getSupportedDepthFormat());
			__createSwapShain();
			__createImageViews();
			__createRenderPass();
			//Ҫ�ڹ��ߴ���ʱ��Ϊ��ɫ���ṩ����ÿ���������󶨵���ϸ��Ϣ,���ǵ����ڹ�����ʹ�ã������Ҫ�ڹ��ߴ���֮ǰ����
			__createDescritorSetLayout();
			__createPipelineCache();
			//Ϊ�����ͼ����֡�����������������ͼ
			__createDepthMapFrameBuffers();

			__createGraphicsPipelines();
			__createCommandPool();

			__createColorResources();
			__createDepthResources();
			__createFrameBuffers();


			__createTextureImage();
			//����������Ҫ������ͼ���ܷ�������
			__createTextureImageView();
			//���ò����������Ժ��ʹ��������ɫ���ж�ȡ��ɫ������û���κεط�����VkImage����������һ�����صĶ������ṩ�˴���������ȡ��ɫ�Ľӿڡ�������Ӧ�����κ���������ͼ���У�������1D��2D��������3D
			__createTextureSampler();

			__createVertexBuffer();
			__createIndexBuffer();
			__createUniformBuffer();

			//������������
			__createDescritorPool();
			__createDescriptorSet();

			__createCommandBuffers();
			__createDepthMapCommandBuffers();

			__createSyncObjects(); //�����ź���
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
			__createFrameBuffers();		  //�����ڽ�����ͼ��
			__createCommandBuffers();	  //�����ڽ�����ͼ��
		}

		//ѭ����Ⱦ
		void __mainLoop()
		{
			while (!glfwWindowShouldClose(pWindow))
			{
				glfwPollEvents();
				__processInput(pWindow);
				//���������еĲ��������첽�ġ���ζ�ŵ������˳�mainLoop��
				//���ƺͳ��ֲ���������Ȼ��ִ�С���������ò��ֵ���Դ�ǲ��Ѻõ�
				//Ϊ�˽��������⣬����Ӧ�����˳�mainLoop���ٴ���ǰ�ȴ��߼��豸�Ĳ������:
				__drawFrame();   //�첽����
				Camera.updateCameraPos();
				//����UniformBuffer
			}
			vkDeviceWaitIdle(m_Device);
		}

		//�����ƶ�
		void __processInput(GLFWwindow* pWindow)
		{
			if (glfwGetKey(pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				glfwSetWindowShouldClose(pWindow, true);
			}

			if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedZ(1.0f);
			}
			else if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedZ(-1.0f);
			}
			else {
				Camera.setKeyBoardSpeedZ(0.0f);
			}


			if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedX(1.0f);
			}
			else if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedX(-1.0f);
			}
			else {
				Camera.setKeyBoardSpeedX(0.0f);
			}


			if (glfwGetKey(pWindow, GLFW_KEY_Q) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedY(1.0f);
			}
			else if (glfwGetKey(pWindow, GLFW_KEY_E) == GLFW_PRESS)
			{
				Camera.setKeyBoardSpeedY(-1.0f);
			}
			else {
				Camera.setKeyBoardSpeedY(0.0f);
			}
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

			vkDestroyPipeline(m_Device, pipelines.graphicsPipeline, nullptr);
			vkDestroyPipeline(m_Device, pipelines.offscreen, nullptr);
			vkDestroyPipelineLayout(m_Device, pipelineLayouts.m_PipelineLayout, nullptr);
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
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);


			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
			vkDestroySampler(m_Device, m_TextureSampler, nullptr);
			vkDestroyImageView(m_Device, m_TextureImageView, nullptr);

			//UBO�����ݽ����������еĻ���ʹ�ã����԰������Ļ�����ֻ����������٣�

				vkDestroyBuffer(m_Device, uniformBuffers.scene, nullptr);
				vkFreeMemory(m_Device, uniformBufferMemorys.scene, nullptr);
				//vkDestroyFence(m_Device, m_ImagesInFlight[i], nullptr);

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
			VCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
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

		// Find a suitable depth format
		VkBool32 __getSupportedDepthFormat()
		{
			VkBool32 validDepthFormat;
			// Since all depth formats may be optional, we need to find a suitable depth format to use
			// Start with the highest precision packed format
			std::vector<VkFormat> depthFormats = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};

			for (auto& format : depthFormats)
			{
				VkFormatProperties formatProps;
				vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
				// Format must support depth stencil attachment for optimal tiling
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					depthFormat = format;
					return true;
				}
			}
			return false;
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
		VkImageView __createImageView(VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags, uint32_t vMipLevels)
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
				m_SwapChainImageViews[i] = __createImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
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
			//��������������֡����֮�󣬱�Ҫ������Ⱦ��֡���帽��
			//��ɫ���帽��
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

			VkAttachmentReference ColorAttachmentRef = {};
			ColorAttachmentRef.attachment = 0;  //ͨ�����������������е�����������,����ֻ��һ����ɫ����
			ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// ��Ȼ��帽��
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//���ز���
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			//��ʹ�ö��ز���ʱ�����ز���ͼ����ֱ����ʾ�������Ƚ����ɳ���ͼ�񣬣���������������Ȼ���������Ҫ����Ϊ��Ҫ��ʾ
			VkAttachmentDescription colorAttachmentResolve = {};
			colorAttachmentResolve.format = m_SwapChainImageFormat;
			colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			VkAttachmentReference colorAttachmentResolveRef = {};
			colorAttachmentResolveRef.attachment = 2;
			colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


			//��ͨ��
			VkSubpassDescription SubpassDescription = {};
			SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //ָ��graphics subpassͼ����ͨ��
			SubpassDescription.colorAttachmentCount = 1;
			SubpassDescription.pColorAttachments = &ColorAttachmentRef;  //ָ����ɫ�������á������������е�����ֱ�Ӵ�Ƭ����ɫ�����ã���layout(location = 0) out vec4 outColor ָ��!
			SubpassDescription.pDepthStencilAttachment = &depthAttachmentRef;
			//SubpassDescription.pResolveAttachments = &colorAttachmentResolveRef;//����Ⱦͨ������һ�������������������������Ⱦͼ����Ļ

			std::vector<VkAttachmentDescription> attachments = { ColorAttachmentDescription, depthAttachment,colorAttachmentResolve };
			//������Ⱦ�ܵ�
			VkRenderPassCreateInfo RenderPassCreateInfo = {};
			RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			RenderPassCreateInfo.pAttachments = attachments.data();
			RenderPassCreateInfo.subpassCount = 1;
			RenderPassCreateInfo.pSubpasses = &SubpassDescription;

			//��ͨ����������Ⱦͨ���е���ͨ�����Զ������ֵı任����Щ�任ͨ����ͨ����������ϵ���п���
			VkSubpassDependency SubpassDependency = {};
			SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;  //����ֵVK_SUBPASS_EXTERNALָ������Ⱦͨ��֮ǰ��֮������Ƿ���srcSubpass��dstSubpass��Ҫ������������ͨ��
			SubpassDependency.dstSubpass = 0;  //������ͨ��������
			//ָ����Ҫ�ȴ��Ĳ�������Щ������ʲô�׶β���������Ҫ�ȵ���������ɴ�ͼ��Ķ�ȡ���ܷ���ͼ�������ͨ���ȴ���ɫ��������׶�������
			SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //ָ���ȴ��Ĳ���������ȴ���������ɶ�Ӧͼ��Ķ�ȡ�����������ͨ���ȴ���ɫ��������Ľ׶���ʵ��
			SubpassDependency.srcAccessMask = 0;

			//�ȴ�����Ĳ���������ɫ�����׶Σ����漰����ȡ��д����ɫ��������Щ���ý�����ֹת�Ƶķ�����ֱ���б�Ҫ���������ʱ��Ҳ����������Ҫ��ʼ����д����ɫ��ʱ��
			SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			RenderPassCreateInfo.dependencyCount = 1;
			RenderPassCreateInfo.pDependencies = &SubpassDependency;

			if (vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}
		}

		//���߻���
		void __createPipelineCache()
		{
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			if (vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache)) {
				throw std::runtime_error("failed to create image!");
			}
		}
		//����ͼ�ι���
		void __createGraphicsPipelines()
		{
			//������Ϣ
			auto BindingDescription = SVertex::getBindingDescription();
			auto AttributeDescription = SVertex::getAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo  PipelinSVertexInputStateCreateInfo = {};
			PipelinSVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			PipelinSVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t> (BindingDescription.size());
			PipelinSVertexInputStateCreateInfo.pVertexBindingDescriptions = BindingDescription.data();
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
			VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo = {};
			PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE; //������Ȳ���
			PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE; //ָ��ͨ����Ȳ��Ե��µ�Ƭ��������ݿ���д����Ȼ�����
			PipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS; //����Ȼ�����������С�Ŀ���ͨ������
			PipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
			PipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
			PipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;
			PipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
			PipelineDepthStencilStateCreateInfo.front = {};
			PipelineDepthStencilStateCreateInfo.back = {};

			//��ɫ����ɫ�����İ�
			VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {};
			PipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
			PipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			PipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			PipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			PipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			//��ɫ
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

			//��̬״̬���ӿڣ�VK_DYNAMIC_STATE_VIEWPORT���Ͳü�����VK_DYNAMIC_STATE_SCISSOR���� ��ʵ��ָ�����ӿںͲü��Ĳ��������ҿ���������ʱ�����ǽ��и��ġ�
			std::vector<VkDynamicState> DynamicState = {
				//�ӿ�
				VK_DYNAMIC_STATE_VIEWPORT,
				//�ü���
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
			DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			DynamicStateCreateInfo.pDynamicStates = DynamicState.data();
			DynamicStateCreateInfo.dynamicStateCount = DynamicState.size();
			DynamicStateCreateInfo.flags = 0;

			//��Ҫ�ڴ������ߵ�ʱ��ָ�����������ϵĲ��֣����Ը�֪Vulkan��ɫ����Ҫʹ�õ��������������������ڹ��߲��ֶ�����ָ��
			VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
			PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutCreateInfo.setLayoutCount = 1; // Optional
			PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional


			if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &pipelineLayouts.m_PipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			//��ɫ��������VkShaderModule����ֻ���ֽ��뻺������һ����װ��������ɫ����û�б˴�����
			//ͨ��VkPipelineShaderStageCreateInfo�ṹ����ɫ��ģ����䵽�����еĶ������Ƭ����ɫ���׶�
			std::vector<VkPipelineShaderStageCreateInfo> ShaderStages = { };
			// Scene rendering with shadows applied
			VkPipelineShaderStageCreateInfo VertStageCreateInfo = __loadShader("./Shaders/ShadowMapVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			ShaderStages.push_back(VertStageCreateInfo);
			VkPipelineShaderStageCreateInfo FragStageCreateInfo = __loadShader("./Shaders/ShadowMapFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			ShaderStages.push_back(FragStageCreateInfo);

			//��������
			VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
			GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			GraphicsPipelineCreateInfo.layout = pipelineLayouts.m_PipelineLayout;   //�Ǿ�����ǽṹ��
			GraphicsPipelineCreateInfo.renderPass = m_RenderPass;
			GraphicsPipelineCreateInfo.subpass = 0;
			//����ͨ���Ѿ����ڵĹ��ߴ����µ�ͼ�ι��ߣ�������ͬ����Լ�ڴ����ģ�
			GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			GraphicsPipelineCreateInfo.basePipelineIndex = -1;
			GraphicsPipelineCreateInfo.pVertexInputState = &PipelinSVertexInputStateCreateInfo;
			GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
			GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
			GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
			GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
			GraphicsPipelineCreateInfo.pDepthStencilState = &PipelineDepthStencilStateCreateInfo; // Optional
			GraphicsPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
			GraphicsPipelineCreateInfo.pDynamicState = nullptr; // Optional
			GraphicsPipelineCreateInfo.stageCount = ShaderStages.size();
			GraphicsPipelineCreateInfo.pStages = ShaderStages.data();

			// Use specialization constants to select between horizontal and vertical blur
			//uint32_t enablePCF = 0;
			//VkSpecializationMapEntry specializationMapEntry{};
			//specializationMapEntry.constantID = 0;
			//specializationMapEntry.offset = 0;
			//specializationMapEntry.size = sizeof(uint32_t);

			//VkSpecializationInfo specializationInfo{};
			//specializationInfo.mapEntryCount = 1;
			//specializationInfo.pMapEntries = &specializationMapEntry;
			//specializationInfo.dataSize = sizeof(uint32_t);
			//specializationInfo.pData = &enablePCF;
			//ShaderStages[1].pSpecializationInfo = &specializationInfo;

			//����ͼ�ι���
			if (vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &GraphicsPipelineCreateInfo, nullptr, &pipelines.graphicsPipeline) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			// PCF filtering
			//enablePCF = 1;
			//if (vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &GraphicsPipelineCreateInfo, nullptr, &pipelines.sceneShadowPCF) != VK_SUCCESS) {
			//	throw std::runtime_error("failed to create graphics pipeline!");
			//}


			//������Ⱦ���ߣ����ͼ��ɫ������
			VkPipelineShaderStageCreateInfo DepthMapVertStageCreateInfo = __loadShader("./Shaders/DepthMapVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			ShaderStages[0] = DepthMapVertStageCreateInfo;
			VkPipelineShaderStageCreateInfo DepthMapFragStageCreateInfo = __loadShader("./Shaders/DepthMapFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			ShaderStages[1] = DepthMapFragStageCreateInfo;

			PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;    //ָ������������ͷ�ʽ
			PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;  		// UnEnable depth bias
			PipelineColorBlendStateCreateInfo.attachmentCount = 0;    // No blend attachment states (no color attachments used)


			DynamicState.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
			DynamicStateCreateInfo.pDynamicStates = DynamicState.data();
			DynamicStateCreateInfo.dynamicStateCount = DynamicState.size();
			DynamicStateCreateInfo.flags = 0;

			GraphicsPipelineCreateInfo.layout = pipelineLayouts.m_PipelineLayout;   //�Ǿ�����ǽṹ��
			GraphicsPipelineCreateInfo.renderPass = DepthMapPass.renderPass;
			if (vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &GraphicsPipelineCreateInfo, nullptr, &pipelines.offscreen) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			//ɾ��
			//vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
			//vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
		}

		//������ɫ��
		VkPipelineShaderStageCreateInfo __loadShader(const std::string& vShaderName, VkShaderStageFlagBits vShaderStageFlagBits)
		{
			VkPipelineShaderStageCreateInfo ShaderStageInfo = {};
			ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			ShaderStageInfo.stage = vShaderStageFlagBits;
			ShaderStageInfo.module = __loadShader(vShaderName);
			ShaderStageInfo.pName = "main";
			return ShaderStageInfo;
		}
		VkShaderModule __loadShader(const std::string& vShaderName)
		{
			auto ShaderCode = __readFile(vShaderName);
			return __createShaderModule(ShaderCode);
		}

		//����֡����
		void __createFrameBuffers()
		{
			//��̬�������ڱ���framebuffers��������С
			m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

			//����ͼ����ͼ��ͨ�����ǽ�����ӦFrameBuffer
			for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
			{		//��Ⱦͨ���������޸�createframebuffer�����µ�ͼ����ͼ��ӵ��б���:
				std::array<VkImageView, 3> attachments = {
						colorImageView,
						depthImageView,
						m_SwapChainImageViews[i]
				};

				VkFramebufferCreateInfo FrameBufferCreateInfo = {};
				FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				FrameBufferCreateInfo.renderPass = m_RenderPass;
				FrameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				FrameBufferCreateInfo.pAttachments = attachments.data();
				FrameBufferCreateInfo.width = m_SwapChainExtent.width;
				FrameBufferCreateInfo.height = m_SwapChainExtent.height;
				FrameBufferCreateInfo.layers = 1;

				if (vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, &m_SwapChainFrameBuffers[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to create Frame Buffer!");
				}
			}
		}

		// Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
		// The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
		//���������ͼ֡����������������ͼ��ֻ��Ҫ������ݣ�����Ҫ��ɫ������ֻҪ��ȸ���
		void __createDepthMapFrameBuffers()
		{
			DepthMapPass.width = SHADOWMAP_SIZE;
			DepthMapPass.height = SHADOWMAP_SIZE;

			//���������ͼ��ֻ��Ҫ��ȸ���
			VkImageCreateInfo image{};
			image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image.imageType = VK_IMAGE_TYPE_2D;
			image.extent.width = DepthMapPass.width;
			image.extent.height = DepthMapPass.height;
			image.extent.depth = 1;
			image.mipLevels = 1;
			image.arrayLayers = 1;
			image.samples = VK_SAMPLE_COUNT_1_BIT;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.format = VK_FORMAT_D16_UNORM;
			// Depth stencil attachment
			image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping

			if (vkCreateImage(m_Device, &image, nullptr, &DepthMapPass.depth.image)) {
				throw std::runtime_error("failed to create depthmap image!");
			}

			//���벢�����ڴ�
			VkMemoryAllocateInfo memAlloc{};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(m_Device, DepthMapPass.depth.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = __findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (vkAllocateMemory(m_Device, &memAlloc, nullptr, &DepthMapPass.depth.mem)) {
				throw std::runtime_error("failed to create image!");
			}
			if (vkBindImageMemory(m_Device, DepthMapPass.depth.image, DepthMapPass.depth.mem, 0)) {
				throw std::runtime_error("failed to create image!");
			}

			//�������ģ����ͼ
			VkImageViewCreateInfo depthStencilView{};
			depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			depthStencilView.format = VK_FORMAT_D16_UNORM;
			depthStencilView.subresourceRange = {};
			depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			depthStencilView.subresourceRange.baseMipLevel = 0;
			depthStencilView.subresourceRange.levelCount = 1;
			depthStencilView.subresourceRange.baseArrayLayer = 0;
			depthStencilView.subresourceRange.layerCount = 1;
			depthStencilView.image = DepthMapPass.depth.image;
			if (vkCreateImageView(m_Device, &depthStencilView, nullptr, &DepthMapPass.depth.view)) {
				throw std::runtime_error("failed to create image!");
			}

			// ������Ȼ�����������������ֱ�Ӷ�ȡ������� 
			// Used to sample in the fragment shader for shadowed rendering
			VkSamplerCreateInfo sampler{};
			sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler.maxAnisotropy = 1.0f;
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = sampler.addressModeU;
			sampler.addressModeW = sampler.addressModeU;
			sampler.mipLodBias = 0.0f;
			sampler.maxAnisotropy = 1.0f;
			sampler.minLod = 0.0f;
			sampler.maxLod = 1.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			if (vkCreateSampler(m_Device, &sampler, nullptr, &DepthMapPass.depthSampler)) {
				throw std::runtime_error("failed to create image!");
			}

			// Ϊ��Ļ����Ⱦ����һ����������Ⱦͨ������Ϊ�����ܲ�ͬ�����ڳ�����Ⱦ��ͨ��
			__createDepthMapRenderpass();

			// Create frame buffer
			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = DepthMapPass.renderPass;
			framebufferCreateInfo.attachmentCount = 1;  //����ֻ��һ�����ģ�建��
			framebufferCreateInfo.pAttachments = &DepthMapPass.depth.view;
			framebufferCreateInfo.width = DepthMapPass.width;
			framebufferCreateInfo.height = DepthMapPass.height;
			framebufferCreateInfo.layers = 1;

			if (vkCreateFramebuffer(m_Device, &framebufferCreateInfo, nullptr, &DepthMapPass.frameBuffer)) {
				throw std::runtime_error("failed to create image!");
			}
		}

		// Set up a separate render pass for the offscreen frame buffer
		// This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
		void __createDepthMapRenderpass()
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = VK_FORMAT_D16_UNORM;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 0;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 0;													// No color attachments
			subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

			// ָ���˵��������ִ�н�������Щ�ź��������źţ�ָ���˵��������ִ�н�������Щ�ź��������ź�se subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies;
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &attachmentDescription;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			if (vkCreateRenderPass(m_Device, &renderPassCreateInfo, nullptr, &DepthMapPass.renderPass)) {
				throw std::runtime_error("failed to create image!");
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

		//����ָ������:�ڼ����úÿڳߴ硢Pipeline��DescriptorSets��VertexBuffer��IndexBuffer����
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

			VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
			CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  //��ָ���ȴ�ִ��ʱ����Ȼ�����ύ��һָ���
			CommandBufferBeginInfo.pInheritanceInfo = nullptr;


			//��Ϊ���������ж���� VK_ATTACHMENT_LOAD_OP_CLEAR �ĸ��������ǻ���Ҫָ��������ֵ
			VkClearValue clearValues[2];
			VkViewport viewport{};
			VkRect2D scissor{};

			//��¼ָ�ָ���
			for (size_t i = 0; i < m_CommandBuffers.size(); i++)
			{
				if (vkBeginCommandBuffer(m_CommandBuffers[i], &CommandBufferBeginInfo) != VK_SUCCESS) {
					throw std::runtime_error("failed to begin recording command buffer!");
				}

				//First render pass: Generate shadow map by rendering the scene from light's POV
				//{
				//	clearValues[0].depthStencil = { 1.0f, 0 };
				//	//��ʼ��Ⱦ����
				//	VkRenderPassBeginInfo RenderPassBeginInfo = {};
				//	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				//	RenderPassBeginInfo.renderPass = DepthMapPass.renderPass;   //ָ����Ⱦ���̶���
				//	RenderPassBeginInfo.framebuffer = DepthMapPass.frameBuffer; //ָ��֡�������
				//	//��Ⱦ���������븽�ű���һ��
				//	RenderPassBeginInfo.renderArea.offset = { 0,0 };
				//	RenderPassBeginInfo.renderArea.extent.width = DepthMapPass.width;
				//	RenderPassBeginInfo.renderArea.extent.height = DepthMapPass.height;
				//	RenderPassBeginInfo.clearValueCount = 1;
				//	RenderPassBeginInfo.pClearValues = clearValues;

				//	vkCmdBeginRenderPass(DepthMapPass.commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				//	//pDynamicState
				//	viewport.width = m_SwapChainExtent.width;
				//	viewport.height = m_SwapChainExtent.height;
				//	viewport.minDepth = 0.0f;
				//	viewport.maxDepth = 1.0f;
				//	vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);

				//	scissor.extent.width = m_SwapChainExtent.width;;
				//	scissor.extent.height = m_SwapChainExtent.height;
				//	scissor.offset = { 0,0 };
				//	vkCmdSetScissor(m_CommandBuffers[i], 0, 1, &scissor);

				//	//TODO
				//	vkCmdBindPipeline(DepthMapPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
				//	vkCmdBindDescriptorSets(DepthMapPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets.offscreen[i], 0, NULL);
				//	vkCmdEndRenderPass(drawCmdBuffers[i]);
				//}

				//Second pass: Scene rendering with applied shadow map

				clearValues[0].color = { 0.1f, 0.1f, 0.3f, 0.5f };;
				clearValues[1].depthStencil = { 1.0f, 0 };

				//��ʼ��Ⱦ����
				VkRenderPassBeginInfo RenderPassBeginInfo = {};
				RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				RenderPassBeginInfo.renderPass = m_RenderPass;   //ָ����Ⱦ���̶���
				RenderPassBeginInfo.framebuffer = m_SwapChainFrameBuffers[i]; //ָ��֡�������
				//��Ⱦ���������븽�ű���һ��
				RenderPassBeginInfo.renderArea.offset = { 0,0 };
				RenderPassBeginInfo.renderArea.extent.width = WIDTH;
				RenderPassBeginInfo.renderArea.extent.height = HEIGHT;
				RenderPassBeginInfo.clearValueCount = 2;
				RenderPassBeginInfo.pClearValues = clearValues;
				//pDynamicState
				viewport.width = WIDTH;
				viewport.height = HEIGHT;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				scissor.extent.width = WIDTH;
				scissor.extent.height = HEIGHT;
				scissor.offset = { 0,0 };

				//������1.ָ������  2.��Ⱦ������Ϣ  3.����Ҫִ�е�ָ�����Ҫָ����У�û�и���ָ�����Ҫִ��
				vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				//vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);
				//vkCmdSetScissor(m_CommandBuffers[i], 0, 1, &scissor);
				//�����������ϰ󶨵�ʵ�ʵ���ɫ������������
				vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.m_PipelineLayout, 0, 1, &m_DescriptorSets, 0, nullptr);
				//��ͼ�ι���:�ڶ�����������ָ�����߶�����ͼ�ι��߻��Ǽ������
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.graphicsPipeline);
				////�󶨶��㻺����
				VkBuffer VertexBuffers[] = { m_VertexBuffer };
				VkDeviceSize  OffSets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, VertexBuffers, OffSets);
				////����������
				vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
				////�ύ���Ʋ�����ָ��壬
				////������1.ָ������������ 2.����instanceing����. 3��û��ʹ��instancing������ָ��1����������ʾ�����ݵ����㻺�����еĶ�������
				////4��ָ��������������ƫ������ʹ��1���ᵼ��ͼ�ο��ڵڶ�����������ʼ��ȡ  5.����������������ӵ�������ƫ�ơ� 6.ָ��instancingƫ����������û��ʹ�ø�����
				vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				//������Ⱦ����
				vkCmdEndRenderPass(m_CommandBuffers[i]);
				//������¼ָ�ָ���
				if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to record command buffer!");
				}
			}

			////������1.ָ������  2.��Ⱦ������Ϣ  3.����Ҫִ�е�ָ�����Ҫָ����У�û�и���ָ�����Ҫִ��
			//vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			////�󶨶��㻺����
			//VkBuffer VertexBuffers[] = { m_VertexBuffer };
			//VkDeviceSize  OffSets[] = { 0 };
			//vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, VertexBuffers, OffSets);

			////����������
			//vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

			////��ͼ�ι���:�ڶ�����������ָ�����߶�����ͼ�ι��߻��Ǽ������
			//vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.graphicsPipeline);

			////�����������ϰ󶨵�ʵ�ʵ���ɫ������������
			//vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.m_PipelineLayout, 0, 1, &descriptorSets.scene[i], 0, nullptr);

			////�ύ���Ʋ�����ָ��壬
			////������1.ָ������������ 2.����instanceing����. 3��û��ʹ��instancing������ָ��1����������ʾ�����ݵ����㻺�����еĶ�������
			////4��ָ��������������ƫ������ʹ��1���ᵼ��ͼ�ο��ڵڶ�����������ʼ��ȡ  5.����������������ӵ�������ƫ�ơ� 6.ָ��instancingƫ����������û��ʹ�ø�����
			//vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}

		void __createDepthMapCommandBuffers()
		{
			if (DepthMapPass.commandBuffer == VK_NULL_HANDLE)
			{
				DepthMapPass.commandBuffer = __createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, false);
			}
			if (DepthMapPass.semaphore == VK_NULL_HANDLE)
			{
				// Create a semaphore used to synchronize offscreen rendering and usage
				VkSemaphoreCreateInfo semaphoreCreateInfo{};
				semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				if (vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &DepthMapPass.semaphore) != VK_SUCCESS) {
					throw std::runtime_error("failed to record command buffer!");
				}
			}

			VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
			CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  //��ָ���ȴ�ִ��ʱ����Ȼ�����ύ��һָ���
			CommandBufferBeginInfo.pInheritanceInfo = nullptr;
			VkClearValue clearValues[1];
			clearValues[0].depthStencil = { 1.0f, 0 };

			//��ʼ��Ⱦ����
			VkRenderPassBeginInfo RenderPassBeginInfo = {};
			RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			RenderPassBeginInfo.renderPass = DepthMapPass.renderPass;   //ָ����Ⱦ���̶���
			RenderPassBeginInfo.framebuffer = DepthMapPass.frameBuffer; //ָ��֡�������
			//��Ⱦ���������븽�ű���һ��
			RenderPassBeginInfo.renderArea.offset = { 0,0 };
			RenderPassBeginInfo.renderArea.extent.width = DepthMapPass.width;
			RenderPassBeginInfo.renderArea.extent.height = DepthMapPass.height;
			RenderPassBeginInfo.clearValueCount = 2;
			RenderPassBeginInfo.pClearValues = clearValues;

			if (vkBeginCommandBuffer(DepthMapPass.commandBuffer, &CommandBufferBeginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}


			vkCmdBeginRenderPass(DepthMapPass.commandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(DepthMapPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(DepthMapPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets.offscreen, 0, NULL);

			//�󶨶��㻺����
			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(DepthMapPass.commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(DepthMapPass.commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(DepthMapPass.commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(DepthMapPass.commandBuffer);

			if (vkEndCommandBuffer(DepthMapPass.commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}

		VkCommandBuffer __createCommandBuffer(VkCommandBufferLevel vLevel, uint32_t vCommandBufferCount, bool begin = false)
		{
			VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
			CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			CommandBufferAllocateInfo.commandPool = m_CommandPool;
			CommandBufferAllocateInfo.level = vLevel;   //���Ա��ύ�����н���ִ�У������ܱ�����ָ��������á�
			CommandBufferAllocateInfo.commandBufferCount = vCommandBufferCount;

			VkCommandBuffer cmdBuffer;
			if (vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}

			// If requested, also start recording for the new command buffer
			if (begin)
			{
				VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
				CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;  //��ָ���ȴ�ִ��ʱ����Ȼ�����ύ��һָ���
				CommandBufferBeginInfo.pInheritanceInfo = nullptr;

				if (vkBeginCommandBuffer(cmdBuffer, &CommandBufferBeginInfo) != VK_SUCCESS) {
					throw std::runtime_error("failed to begin recording command buffer!");
				}
			}
			return cmdBuffer;
		}
		//���Ʋ��裺��Ҫͬ��������դ��������Ӧ�ó�������Ⱦ��������ͬ���������ź�������������������ڻ��߿��������ͬ��������ʵ��ͬ��
		//1.���ȣ��ӽ������л�ȡһͼ��
		//2.��֡������ʹ����Ϊ������ͼ��ִ����������е�����NDLE,
		//3.��Ⱦ���ͼ�񷵻ؽ�����
		//Ϊʲô��Ҫͬ���������������趼���첽����ģ�û��һ����˳��ִ�У�ʵ����ÿһ������Ҫ��һ���Ľ���������У������Ҫ�����ź�����ͬ��
		void __drawFrame()
		{
			//��ǰһ֡���
			//vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

			uint32_t ImageIndex;
			//1.���ȣ��ӽ������л�ȡһͼ��,
			//���ĸ���������presentation���������ͼ��ĳ��ֺ��ʹ�øö������źš�����ǿ�ʼ���Ƶ�ʱ��㡣
			VkResult Result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, semaphores.presentComplete, VK_NULL_HANDLE, &ImageIndex);
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
			//if (m_ImagesInFlight[ImageIndex] != VK_NULL_HANDLE) {
			//	vkWaitForFences(m_Device, 1, &m_ImagesInFlight[ImageIndex], VK_TRUE, UINT64_MAX);
			//}
			//��۵�ǰ֡����ʹ�����ͼ��
			//m_ImagesInFlight[ImageIndex] = m_InFlightFences[m_CurrentFrame];


			//2.�ύ����嵽ͼ�ζ���
			//VkSubmitInfo SubmitInfo = {};
			SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			//��ʹ����Ӱ��ͼ֮ǰ��������Ⱦ�����������ȴ���Ļ�����Ⱦ(�ʹ���)���
			// Offscreen rendering
			//���ź������һ��ͼ���Ѿ���ȡ����׼����Ⱦ������WaitStages��ʾ�������Ǹ��׶εȴ�
			//VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			SubmitInfo.pWaitDstStageMask = waitStages;
			// Wait for swap chain presentation to finish
			SubmitInfo.pWaitSemaphores = &semaphores.presentComplete;
			// Signal ready with offscreen semaphore
			//ָ���˵��������ִ�н�������Щ�ź��������ź�
			SubmitInfo.pSignalSemaphores = &DepthMapPass.semaphore;//signalSemaphoreCount��pSignalSemaphores����ָ���˵��������ִ�н�������Щ�ź��������ź�

			// Submit work
			SubmitInfo.commandBufferCount = 1;
			SubmitInfo.pCommandBuffers = &DepthMapPass.commandBuffer;
			if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer!");
			}

			// Scene rendering
			//ָ���˵��������ִ�н�������Щ�ź��������źš�������Ҫʹ��renderFinishedSemaphore����Ⱦ�Ѿ���ɿ��Գ�����
			//VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
			// Wait for offscreen semaphore
			SubmitInfo.pWaitSemaphores = &DepthMapPass.semaphore;;
			// Signal ready with render complete semaphpre
			SubmitInfo.pSignalSemaphores = &semaphores.renderComplete;

			// Submit work
			SubmitInfo.pCommandBuffers = &m_CommandBuffers[ImageIndex];
			if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
				throw std::runtime_error("failed to submit draw command buffer!");
			}


			////���ź������һ��ͼ���Ѿ���ȡ����׼����Ⱦ������WaitStages��ʾ�������Ǹ��׶εȴ�
			//VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
			//VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			//SubmitInfo.waitSemaphoreCount = 1;
			////ִ�п�ʼ֮ǰҪ�ȴ����ĸ��ź�����Ҫ�ȴ���ͨ�����ĸ��׶�
			//SubmitInfo.pWaitSemaphores = WaitSemaphores;
			//SubmitInfo.pWaitDstStageMask = WaitStages;
			////ָ���ĸ����������ʵ���ύִ�У�����Ϊ�ύ����������������Ǹջ�ȡ�Ľ�����ͼ����Ϊ��ɫ�������а󶨡�
			//SubmitInfo.commandBufferCount = 1;
			//SubmitInfo.pCommandBuffers = &m_CommandBuffers[ImageIndex];

			////ָ���˵��������ִ�н�������Щ�ź��������źš�������Ҫʹ��renderFinishedSemaphore����Ⱦ�Ѿ���ɿ��Գ�����
			//VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
			//SubmitInfo.signalSemaphoreCount = 1;
			////SubmitInfo.pSignalSemaphores = SignalSemaphores;

			////������������
			//vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

			////����������ύ��ͼ�ζ���
			////����������ִ�е�ʱ��Ӧ�ñ�ǵ�դ�������ǿ�������������һ��֡�Ѿ������
			//if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
			//	throw std::runtime_error("failed to submit draw command buffer!");
			//}

			//������������
			//vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

			//3.������ύ����������ʹ��������ʾ����Ļ��
			VkPresentInfoKHR PresentInfo = {};
			PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			PresentInfo.waitSemaphoreCount = 1;
			//ָ���ڽ���presentation֮ǰҪ�ȴ����ź���
			PresentInfo.pWaitSemaphores = &semaphores.renderComplete;

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

		//ͬ��ԭ�1.�����ź����������ź������ϣ�Ϊ�������֡  2.Fence
		void __createSyncObjects()
		{
			//m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			//m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			//m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			//m_ImagesInFlight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			//VkFenceCreateInfo fenceInfo = {};
			//fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			// Create a semaphore used to synchronize image presentation
            // Ensures that the image is displayed before we start submitting new commands to the queu
			if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &semaphores.presentComplete)) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
			// Create a semaphore used to synchronize command submission
			// Ensures that the image is not presented until all commands have been sumbitted and executed
			if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &semaphores.renderComplete)) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
			//for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			//	if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			//		vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
			//		vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
			//		throw std::runtime_error("failed to create synchronization objects for a frame!");
			//	}
			//}
			SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			SubmitInfo.pWaitDstStageMask = &submitPipelineStages;
			SubmitInfo.waitSemaphoreCount = 1;
			SubmitInfo.pWaitSemaphores = &semaphores.presentComplete;
			SubmitInfo.signalSemaphoreCount = 1;
			SubmitInfo.pSignalSemaphores = &semaphores.renderComplete;

		}

		//�������㻺��,������ɫ������֮��ʹ��CPU�ɼ��Ļ�����Ϊ��ʱ���壬ʹ���Կ���ȡ�Ͽ�Ļ�����Ϊ�����Ķ��㻺��
		//���̣����ڶ��㻺��/�������壬��Ҫ���ڶ�������CPU)�ɼ��Ĺ����ڴ��Ϸ�����ʱ�ڴ�stagingBuffer��������copy�����ڴ棬Ȼ����GPU�Ϸ���������ڴ棬ͨ��Transfer Command�����ݴӹ����ڴ���߸��ٻ��濽����GPU�ڴ档
		void __createVertexBuffer()
		{
			VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

			//�ݴ滺����
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
		void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout VNewLayout, uint32_t vMipLevels)
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
			else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && VNewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
				ImageMemoryBarrier.srcAccessMask = 0;
				ImageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				DestinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else {
				throw std::invalid_argument("unsupported layout transition!");
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

		

		//uniform������
		//��ÿһ֡��������Ҫ�����µ����ݵ�UBO�����������Դ���һ���ݴ滺������û������ġ�����������£���ֻ�����Ӷ���Ŀ��������ҿ��ܽ������ܶ������������ܡ�
		void __createUniformBuffer()
		{

			for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
				__createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers.scene, uniformBufferMemorys.scene);
				// Offscreen vertex shader uniform buffer block
				__createBuffer(sizeof(LSM), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers.offscreen, uniformBufferMemorys.offscreen);
				__updateUniformBuffer(i);
			}
		}

		//����uniform����
		//�ú�������ÿһ֡�д����µı任������ȷ������ͼ����ת��������Ҫ�����µ�ͷ�ļ�ʹ�øù��ܣ�
		void __updateUniformBuffer(uint32_t vCurrentImage)
		{
			//������ת������Դ����ϵ��
			//����ͶӰ
			glm::mat4 DepthProjectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 10.0f, 10.0f);
			glm::mat4 DepthViewMatrix = glm::lookAt(LightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
			glm::mat4 DepthModelMatrix = glm::mat4(1.0f);
			//��ÿ������ռ�����任����Դ�����������Ǹ��ռ�(�ü��ռ䣩
			LSM.LightSpaceVP = DepthProjectionMatrix * DepthViewMatrix;
			LSM.ModelMatrix = DepthModelMatrix;

			void* OffscreenData;
			//ӳ���豸�ڴ�
			vkMapMemory(m_Device, uniformBufferMemorys.offscreen, 0, sizeof(LSM), 0, &OffscreenData);
			memcpy(OffscreenData, &LSM, sizeof(LSM));
			vkUnmapMemory(m_Device, uniformBufferMemorys.offscreen);

			static auto StartTime = std::chrono::high_resolution_clock::now();

			auto CurrentTime = std::chrono::high_resolution_clock::now();
			float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

			UniformBufferObject UBO = {};
			UBO.AmbientStrenght = AmbientStrenght;
			UBO.SpecularStrenght = SpecularStrenght;
			UBO.BaseLight = BaseLight;
			UBO.LightPos = LightPos;
			UBO.LightDirection = LightDirection;
			UBO.ViewPos = Camera.getCameraPos();
			//�۹��
			UBO.FlashPos = Camera.getCameraPos();
			UBO.FlashDir = Camera.getCameraDir();
			UBO.FlashPos = FlashColor;
			UBO.OuterCutOff = glm::cos(glm::radians(10.5f));
			UBO.InnerCutOff = glm::cos(glm::radians(8.5f));

			UBO.ModelMatrix = glm::mat4(1.0f);
			//glm::lookAt �������۾�λ�ã�����λ�ú��Ϸ���Ϊ������
			UBO.ViewMatrix = Camera.getViewMatrix();
			//ѡ��ʹ��FOVΪ45�ȵ�͸��ͶӰ�����������ǿ�߱ȣ����ü����Զ�ü��档��Ҫ����ʹ�õ�ǰ�Ľ�������չ�������߱ȣ��Ա��ڴ��������С��ο����µĴ����Ⱥ͸߶ȡ�
			UBO.ProjectiveMatrix = glm::perspective(glm::radians(Fov), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, ZNear, ZFar);
			//GLM�����ΪOpenGL��Ƶģ����Ĳü������Y�Ƿ�ת�ġ��������������򵥵ķ�������ͶӰ������Y����������ӷ�ת
			UBO.ProjectiveMatrix[1][1] *= -1;
			UBO.LightSpaceMatrix = LSM.LightSpaceVP;

			//�Խ�UBO�е����ݸ��Ƶ�uniform������
			void* Data;
			//ӳ���豸�ڴ�
			vkMapMemory(m_Device, uniformBufferMemorys.scene, 0, sizeof(UBO), 0, &Data);
			memcpy(Data, &UBO, sizeof(UBO));
			vkUnmapMemory(m_Device, uniformBufferMemorys.scene);
		}

		//������������
		void __createDescritorPool()
		{
			//���ͼ�������������
			std::array<VkDescriptorPoolSize, 3> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = 6;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 4;
			poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[2].descriptorCount = 4;

			//�����������صĴ�С
			VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
			DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			DescriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
			DescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			DescriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());  //ָ�����Է���������������������

			if (vkCreateDescriptorPool(m_Device, &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create descriptor pool!");
			}
		}
		//ÿ���󶨶���ͨ��VkDescriptorSetLayoutBinding�ṹ������
		void __createDescritorSetLayout()
		{
			//������ɫ��binding 0��UBO������
			VkDescriptorSetLayoutBinding  UBOLayoutBinding = {};
			UBOLayoutBinding.binding = 0;                                         //��ɫ����ʹ�õ�binding
			UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  //ָ����ɫ��������������
			UBOLayoutBinding.descriptorCount = 1;                                 //ָ����������ֵ������MVP�任Ϊ��UBO�������Ϊ1
			UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;             //ָ������������ɫ���ĸ��׶α����ã�������ڶ�����ɫ����ʹ��������
			UBOLayoutBinding.pImmutableSamplers = nullptr;                        //��ͼ��������������й�

			//Ƭ����ɫ��binding 1����һ�������������ͼ��ȡ����(combined image sampler)
			VkDescriptorSetLayoutBinding SamplerLayoutBinding1{};
			SamplerLayoutBinding1.binding = 1;
			SamplerLayoutBinding1.descriptorCount = 1;
			SamplerLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			SamplerLayoutBinding1.pImmutableSamplers = nullptr;
			//ָ����Ƭ����ɫ����ʹ�����ͼ�������������
			SamplerLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			//Ƭ����ɫ��binding 2:�����ͼ������
			VkDescriptorSetLayoutBinding SamplerLayoutBinding2{};
			SamplerLayoutBinding2.binding = 2;
			SamplerLayoutBinding2.descriptorCount = 1;
			SamplerLayoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			SamplerLayoutBinding2.pImmutableSamplers = nullptr;
			//ָ����Ƭ����ɫ����ʹ�����ͼ�������������
			SamplerLayoutBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::vector<VkDescriptorSetLayoutBinding> bindings = { UBOLayoutBinding, SamplerLayoutBinding1, SamplerLayoutBinding2 };
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout!");
			}

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = 1;
			pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;

			// screen pipeline layout
			if (vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen)) {
				throw std::runtime_error("failed to create!");
			}
		}

		//�и��������ؾͿ��Կ�ʼ������������������
		// ���һ���ǽ�ʵ�ʵ�ͼ��Ͳ�������Դ�󶨵������������еľ���������
		void __createDescriptorSet()
		{

			VkDescriptorSetLayout DescriptorSetLayouts = {m_DescriptorSetLayout };

			VkDescriptorSetAllocateInfo AllocateInfo = {};
			AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocateInfo.descriptorPool = m_DescriptorPool;
			AllocateInfo.descriptorSetCount = 1;
			AllocateInfo.pSetLayouts = &DescriptorSetLayouts;

			//����Ҫ��ȷ�������������ϣ���Ϊ���ǻ�����������������ٵ�ʱ���Զ���
			//���������ط���������������
			if (vkAllocateDescriptorSets(m_Device, &AllocateInfo, &m_DescriptorSets) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate descriptor set!");
			}

			// Image descriptor for the shadow map attachment
            //���������Ϸ����֮����Ҫ�����������ã�ָ�����û�����
			VkDescriptorImageInfo descriptorImageInfo = {};
			descriptorImageInfo.sampler = DepthMapPass.depthSampler;
			descriptorImageInfo.imageView = DepthMapPass.depth.view;
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


	 		//�����������󴴽��󣬻���Ҫ����һ�������á�ʹ��ѭ�������������������󣬶����������ã�
	        //off_screen������Ⱦ
			VkDescriptorBufferInfo DescriptorBufferInfo = {};
			DescriptorBufferInfo.buffer = uniformBuffers.offscreen;
			DescriptorBufferInfo.offset = 0;
			DescriptorBufferInfo.range = sizeof(LSM);


			//�����������ø���ʹ��vkUpdateDescriptorSets����������ҪVkWriteDescriptorSet�ṹ���������Ϊ������
			std::vector<VkWriteDescriptorSet>	writeDescriptorSets = {};
			writeDescriptorSets.resize(2);
			// Binding 0 : Vertex shader uniform buffer
			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = m_DescriptorSets;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
			writeDescriptorSets[0].descriptorCount = 1;

			// Binding 1 : Fragment shader texture sampler
			writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[1].dstSet = m_DescriptorSets;
			writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[1].dstBinding = 1;
			writeDescriptorSets[1].pImageInfo = &descriptorImageInfo;
			writeDescriptorSets[1].descriptorCount = 1;
			vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);


			//
			if (vkAllocateDescriptorSets(m_Device, &AllocateInfo, &descriptorSets.offscreen) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set!");
			}
			// Offscreen shadow map generation������Ӱ��ͼ
            // Binding 0 : Vertex shader uniform buffer
			writeDescriptorSets.resize(1);
			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = descriptorSets.offscreen;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
			writeDescriptorSets[0].descriptorCount = 1;
			vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);



			if (vkAllocateDescriptorSets(m_Device, &AllocateInfo, &descriptorSets.scene) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set!");
			}
				//3D scene ������
			   // Image descriptor for the shadow map attachment
				descriptorImageInfo.sampler = DepthMapPass.depthSampler;
				descriptorImageInfo.imageView = DepthMapPass.depth.view;

				writeDescriptorSets.resize(2);
				// Binding 0 : Vertex shader uniform buffer
				writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[0].dstSet = descriptorSets.scene;
				writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[0].dstBinding = 0;
				writeDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
				writeDescriptorSets[0].descriptorCount = 1;

				// Binding 1 : Fragment shader texture sampler
				writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[1].dstSet = descriptorSets.scene;
				writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[1].dstBinding = 1;
				writeDescriptorSets[1].pImageInfo = &descriptorImageInfo;
				writeDescriptorSets[1].descriptorCount = 1;
				vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);



				//3D ����
				VkDescriptorBufferInfo DescriptorBuffer{};
				DescriptorBuffer.buffer = uniformBuffers.scene;
				DescriptorBuffer.offset = 0;
				DescriptorBuffer.range = sizeof(UniformBufferObject);

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = m_TextureImageView;
				imageInfo.sampler = m_TextureSampler;

				writeDescriptorSets.resize(3);
				// Binding 0 : Vertex shader uniform buffer
				writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[0].dstSet = m_DescriptorSets ;
				writeDescriptorSets[0].dstBinding = 0;                  //Ϊ uniform buffer �󶨵������趨Ϊ0
				writeDescriptorSets[0].dstArrayElement = 0;             //���������������飬������Ҫָ��Ҫ���µ�����������������û��ʹ�����飬���Լ򵥵�����Ϊ0
				writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; //ָ������������
				writeDescriptorSets[0].descriptorCount = 1;             //ָ������������������Ҫ������
				writeDescriptorSets[0].pBufferInfo = &DescriptorBuffer;      //ָ�����������õĻ��������� 

				// Binding 1 : Fragment shader shadow sampler
				writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[1].dstSet = m_DescriptorSets;
				writeDescriptorSets[1].dstBinding = 1;
				writeDescriptorSets[1].dstArrayElement = 0;
				writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[1].descriptorCount = 1;
				writeDescriptorSets[1].pImageInfo = &imageInfo;   ////ָ�����������õ�ͼ������

				// Binding 2 : Fragment shader DepthMap sampler
				writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[2].dstSet = m_DescriptorSets;
				writeDescriptorSets[2].dstBinding = 2;
				writeDescriptorSets[2].dstArrayElement = 0;
				writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[2].descriptorCount = 1;
				writeDescriptorSets[2].pImageInfo = &descriptorImageInfo;   ////ָ�����������õ�ͼ������

				//����������������
				vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
				//������Ҫ����__createCommandBuffers������ʹ��cmdBindDescriptorSets�����������ϰ󶨵�ʵ�ʵ���ɫ�����������У�
			
		}

		//����ͼƬ���ύ��VulKanͼ������У�;�л��õ���������������__createCommandPool����֮��
		void __createTextureImage()
		{
			int TexWidth, TexHeight, TexChannels;

			//STBI_rgb_alphaֵǿ�Ƽ���ͼƬ��alphaͨ������ʹû�и�ͨ��ֵ�������������У�ÿ���������ĸ��ֽ�
			stbi_uc* Pixels = stbi_load("./Images/viking_room.png", &TexWidth, &TexHeight, &TexChannels, STBI_rgb_alpha);
			VkDeviceSize ImageSize = TexWidth * TexHeight * 4;

			if (!Pixels) {
				throw std::runtime_error("failed to load texture image!");
			}

			VkBuffer StagingBuffer;
			VkDeviceMemory StagingBufferMemory;

			//������ʱ������
			__createBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				StagingBuffer, StagingBufferMemory);

			//ʹ��ӳ�䣨vkMapMemory���ͽ�ӳ�䣨vkUnmapMemory��������ͼ������ push �� GPU �ϡ�
			void* data;
			//��GPUӳ�䵽host�ڴ���
			vkMapMemory(m_Device, StagingBufferMemory, 0, ImageSize, 0, &data);
			//������������copy��GPU
			memcpy(data, Pixels, static_cast<size_t>(ImageSize));
			vkUnmapMemory(m_Device, StagingBufferMemory);

			//��Ҫ��������ԭͼ�����������
			stbi_image_free(Pixels);
			__createImage(TexWidth, TexHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_TextureImage, m_TextureImageMemory);

			//������ͼͼ����һ��copy�ݴ滺��������ͼͼ��
			//1.�任��ͼͼ�� VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			__transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
			//2.ִ�л�������ͼ��Ŀ�������
			__copyBufferToImage(StagingBuffer, m_TextureImage, static_cast<uint32_t>(TexWidth), static_cast<uint32_t>(TexHeight));
			//��shader��ɫ���п�ʼ����ͼͼ��Ĳ�����������Ҫ���һ���任��׼����ɫ������
			__transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

			vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
			vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
		}

		//����ͼ�������ڴ����
		void __createImage(uint32_t vWidth, uint32_t vHeight, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage,
			VkMemoryPropertyFlags vProperties, VkImage& vImage, VkDeviceMemory& vImageMemory)
		{
			//������ͨ����ɫ�����ʻ�����������ֵ������VuklKam�����ʹ��image������ʻ����������Կ��ټ�����ɫ
			VkImageCreateInfo ImageCreateInfo = {};
			ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                   //ָ��ͼ�����ͣ���֪Vulkan����ʲô��������ϵ��ͼ���вɼ����ء���������1D��2D��3Dͼ��1Dͼ�����ڴ洢�������ݻ��߻Ҷ�ͼ��2Dͼ����Ҫ����������ͼ��3Dͼ�����ڴ洢�������ء�
			ImageCreateInfo.extent.width = static_cast<uint32_t>(vWidth); //ͼ��ߴ�
			ImageCreateInfo.extent.height = static_cast<uint32_t>(vHeight);
			ImageCreateInfo.extent.depth = 1;
			ImageCreateInfo.mipLevels = 1;
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
			m_TextureImageView = __createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		//��ɫ��ֱ�Ӵ�ͼ���ж�ȡ�����ǿ��Եģ����ǵ�������Ϊ����ͼ���ʱ�򲢲�����������ͼ��ͨ��ʹ�ò���������
		//��������������
		void __createTextureSampler()
		{
			VkSamplerCreateInfo SamplerInfo = {};
			//ָ���������ͱ任
			SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			SamplerInfo.magFilter = VK_FILTER_LINEAR;   //ָ�����طŴ��ڲ�ֵ��ʽ����ʹ����ӽ����������������Χ���ĸ����صļ�Ȩƽ��ֵ��
			SamplerInfo.minFilter = VK_FILTER_LINEAR;   //ָ��������С�ڲ�ֵ��ʽ

			//ָ��ÿ������ʹ�õ�Ѱַģʽ������ռ�����UVW��Ӧ��XYZ
			SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  //��ͼ������Ԫ�����곬�� [0 .. 1] ��Χʱ�����ֶο����� U �����ͼ���ơ� ��
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
			SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS; //��ִ������Ĺ���֮ǰ������ʹ�ô��ֶ���ָ���ıȽϺ����Ƚ�ȡ��������Ԫ�����ݡ� 

			//mipmapping
			SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			SamplerInfo.mipLodBias = 0.0f;
			SamplerInfo.minLod = 0.0f;
			SamplerInfo.maxLod = 0.0f;

			if (vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS) {
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		void __createColorResources() {
			VkFormat colorFormat = m_SwapChainImageFormat;

			__createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
			colorImageView = __createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			__transitionImageLayout(colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
		}

		void __createDepthResources() {
			__createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
			depthImageView = __createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}
		
	};
}