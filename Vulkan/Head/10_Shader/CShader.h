#include<vulkan\vulkan.h>
#include "../../Head/1_Device/CLogicalDevice.h"
#include "../../Head/2_Buffer/CBuffer.h"

namespace spirv_cross
{
	class Compiler;
	struct ShaderResources;
}

//Function:VkShaderModule对象对着色器字节码的包装
namespace vk_Demo
{
	class CShaderModule
	{
	public:
		~CShaderModule();

		//static指对函数的作用域仅局限于本文件(所以又称内部函数)
		static CShaderModule* createShaderModule(std::shared_ptr<CLogicalDevice> vDevice, const std::string& vFileName, VkShaderStageFlagBits vStageFlag);
		VkShaderModule getInstanceHandle() const { return m_ShaderModule; };
		VkShaderStageFlagBits getShaderStageFlage() const { return m_ShaderStageFlag; };

		VkDevice              m_Device;
		VkShaderModule        m_ShaderModule;
		VkShaderStageFlagBits m_ShaderStageFlag; //shader用于pipeline哪个阶段
		uint32_t*              m_Data;
		uint32_t              m_DataSize;

	private:
		CShaderModule() {};
	};

	class CShader
	{
	public:
		struct BufferInfo
		{
			uint32_t				set = 0;
			uint32_t				binding = 0;
			uint32_t				bufferSize = 0;
			VkDescriptorType	    descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VkShaderStageFlags	    stageFlags = 0;
		};

		struct ImageInfo
		{
			uint32_t				set = 0;
			uint32_t				binding = 0;
			VkDescriptorType	    descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			VkShaderStageFlags	    stageFlags = 0;
		};

		~CShader();
	
		static CShader* createShader(std::shared_ptr<CLogicalDevice> vulkanDevice, bool dynamicUBO, const char* vert, const char* frag, const char* geom = nullptr, const char* comp = nullptr, const char* tesc = nullptr, const char* tese = nullptr);    

	private:
		CShaderModule*				vertShaderModule = nullptr;
		CShaderModule*				fragShaderModule = nullptr;
		CShaderModule*				geomShaderModule = nullptr;
		CShaderModule*				compShaderModule = nullptr;
		CShaderModule*				tescShaderModule = nullptr;
		CShaderModule*				teseShaderModule = nullptr;

		VkDevice						device = VK_NULL_HANDLE;
		bool                            dynamicUBO = false;
		CDescriptorSetLayoutsInfo		setLayoutsInfo;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
		std::vector<SAttribute>         m_InputAttributes;
		std::vector<VertexAttribute>	perVertexAttributes;
		std::vector<VertexAttribute>    instancesAttributes;
		std::vector<VkVertexInputBindingDescription>     inputBindings;
		std::vector<VkVertexInputAttributeDescription>   inputAttributes;

		std::vector<VkDescriptorSetLayout> 			descriptorSetLayouts;
		VkPipelineLayout 				            pipelineLayout = VK_NULL_HANDLE;
		std::vector<CDescriptorSetPool*>			descriptorSetPools;
		std::unordered_map<std::string, BufferInfo>	bufferParams;
		std::unordered_map<std::string, ImageInfo>	imageParams;

		void __compile();
		void __processShaderModule(CShaderModule* shaderModule);
		void __processUniformBuffers(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags);
		void __processTextures(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags);
		void __processInput(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags);
		void __generateInputInfo();
		void __generateLayout();

		CShader() {};
	};


	//分配set
	class CDescriptorSetPool
	{
	public:
		CDescriptorSetPool(VkDevice inDevice, uint32_t inMaxSet, const CDescriptorSetLayoutsInfo& setLayoutsInfo, const std::vector<VkDescriptorSetLayout>& inDescriptorSetLayouts);
		~CDescriptorSetPool();

		bool allocateDescriptorSet(VkDescriptorSet* descriptorSet);
	private:
		VkDevice						device = VK_NULL_HANDLE;
		VkDescriptorPool                descriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout>  descriptorSetLayouts;
		uint32_t                        maxSet;
		uint32_t                        usedSet;
	};

	struct SAttribute
	{
		VertexAttribute	attribute;
		uint32_t			location;
	};

	//描述符集合分配好之后，配置描述符，更新描述符集，从而将描述符集与资源信息关联起来
	class CDescriptorSet
	{
	public:
		CDescriptorSet() {};
		~CDescriptorSet() {};

		void writeBuffer(const std::string& name, const VkDescriptorBufferInfo* bufferInfo);
		void writeBuffer(const std::string& name, const CBuffer* bufferInfo);
		void writeImage(const std::string& name, DVKTexture* texture);
	private:
		VkDevice						device = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet>    descriptorSets;
		CDescriptorSetLayoutsInfo setLayoutsInfo;
	};

	class CDescriptorSetLayoutInfo
	{
	public:
		CDescriptorSetLayoutInfo() {};
		~CDescriptorSetLayoutInfo() {};

		uint32_t getSet() const { return set; };
		std::vector<VkDescriptorSetLayoutBinding> getBinding() const { return bindings; };

		uint32_t			set = -1;
		std::vector<VkDescriptorSetLayoutBinding>	bindings;
	};

	class CDescriptorSetLayoutsInfo
	{
	public:
		struct BindInfo
		{
			uint32_t set;
			uint32_t binding;
		};

		CDescriptorSetLayoutsInfo() {};
		~CDescriptorSetLayoutsInfo() {};

		VkDescriptorType GetDescriptorType(uint32_t set, uint32_t binding);
		void AddDescriptorSetLayoutBinding(const std::string& varName, uint32_t set, VkDescriptorSetLayoutBinding binding);
	public:
		std::unordered_map<std::string, BindInfo>	paramsMap;
		std::vector<CDescriptorSetLayoutInfo>		setLayouts;
	};
}