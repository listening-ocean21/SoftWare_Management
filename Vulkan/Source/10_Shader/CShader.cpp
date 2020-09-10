#include <fstream>
#include <string>

#include"../../Head/10_Shader/CShader.h"
#include "SPIRV-Cross/spirv_cross.hpp"

namespace vk_Demo
{
	CShaderModule::~CShaderModule()
	{
		if (m_ShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
		}

		if (m_Data)
		{
			delete[] m_Data;
			m_Data = nullptr;
		}
	}
	//static
	CShaderModule* CShaderModule::createShaderModule(std::shared_ptr<CLogicalDevice> vDevice, const std::string& vFileName, VkShaderStageFlagBits vStageFlag)
	{
		VkDevice Device = vDevice->getInstanceHandle();

		std::ifstream File(vFileName, std::ios::ate | std::ios::binary);
		if (!File.is_open())
		{
			throw std::runtime_error("Failed to open ShaderCode File!");
		}

		size_t FileSize = (size_t)File.tellg();
		std::vector<char> ShaderCodeBuffer(FileSize);
		File.seekg(0);
		File.read(ShaderCodeBuffer.data(), FileSize);
		File.close();

		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = ShaderCodeBuffer.size();
		//Shadercode字节码是以字节为指定，但字节码指针是uint32_t类型，因此需要转换类型
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(ShaderCodeBuffer.data());

		//创建vkShaderModule
		VkShaderModule TempShaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &TempShaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create ShaderModule!");
		}

		CShaderModule* ShaderModule     = new CShaderModule();
		ShaderModule->m_ShaderModule    = TempShaderModule;
		ShaderModule->m_Device          = Device;
		ShaderModule->m_ShaderStageFlag = vStageFlag;
		ShaderModule->m_Data            = reinterpret_cast<uint32_t*>(ShaderCodeBuffer.data());
		ShaderModule->m_DataSize        = FileSize;

		return ShaderModule;
	}

	CShader::~CShader() 
	{
		if (vertShaderModule)
		{
			delete vertShaderModule;
			vertShaderModule = nullptr;
		}

		if (fragShaderModule)
		{
			delete fragShaderModule;
			fragShaderModule = nullptr;
		}

		if (geomShaderModule)
		{
			delete geomShaderModule;
			geomShaderModule = nullptr;
		}

		if (compShaderModule)
		{
			delete compShaderModule;
			compShaderModule = nullptr;
		}

		if (tescShaderModule)
		{
			delete tescShaderModule;
			tescShaderModule = nullptr;
		}

		if (teseShaderModule)
		{
			delete teseShaderModule;
			teseShaderModule = nullptr;
		}

		for (uint32_t i = 0; i < descriptorSetLayouts.size(); ++i) {
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[i], nullptr);
		}
		descriptorSetLayouts.clear();

		if (pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}

		for (uint32_t i = 0; i < descriptorSetPools.size(); ++i) {
			delete descriptorSetPools[i];
		}
		descriptorSetPools.clear();
	}

	static CShader* CShader::createShader(std::shared_ptr<CLogicalDevice> vulkanDevice, const char* comp);
	static CShader* CShader::createShader(std::shared_ptr<CLogicalDevice> vulkanDevice, const char* vert, const char* frag, const char* geom = nullptr, const char* comp = nullptr, const char* tesc = nullptr, const char* tese = nullptr);

	CShader* CShader::createShader(std::shared_ptr<CLogicalDevice> vulkanDevice, bool dynamicUBO, const char* vert, const char* frag, const char* geom, const char* comp, const char* tesc, const char* tese)
	{
		CShaderModule* vertModule = vert ? CShaderModule::createShaderModule(vulkanDevice, vert, VK_SHADER_STAGE_VERTEX_BIT) : nullptr;
		CShaderModule* fragModule = frag ? CShaderModule::createShaderModule(vulkanDevice, frag, VK_SHADER_STAGE_FRAGMENT_BIT) : nullptr;
		CShaderModule* geomModule = geom ? CShaderModule::createShaderModule(vulkanDevice, geom, VK_SHADER_STAGE_GEOMETRY_BIT) : nullptr;
		CShaderModule* compModule = comp ? CShaderModule::createShaderModule(vulkanDevice, comp, VK_SHADER_STAGE_COMPUTE_BIT) : nullptr;
		CShaderModule* tescModule = tesc ? CShaderModule::createShaderModule(vulkanDevice, tesc, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) : nullptr;
		CShaderModule* teseModule = tese ? CShaderModule::createShaderModule(vulkanDevice, tese, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) : nullptr;

		CShader* shader = new CShader();
		shader->device = vulkanDevice->getInstanceHandle();
		shader->dynamicUBO = dynamicUBO;
		shader->vertShaderModule = vertModule;
		shader->fragShaderModule = fragModule;
		shader->geomShaderModule = geomModule;
		shader->compShaderModule = compModule;
		shader->tescShaderModule = tescModule;
		shader->teseShaderModule = teseModule;

		//解析Layout等信息
		shader->__compile();

		return shader;
	}
	CDescriptorSet* CShader::allocateDescriptorSet()
	{
		if (setLayoutsInfo.setLayouts.size() == 0) {
			return nullptr;
		}

		DVKDescriptorSet* dvkSet = new DVKDescriptorSet();
		dvkSet->device = device;
		dvkSet->setLayoutsInfo = setLayoutsInfo;
		dvkSet->descriptorSets.resize(setLayoutsInfo.setLayouts.size());

		for (int32 i = descriptorSetPools.size() - 1; i >= 0; --i)
		{
			if (descriptorSetPools[i]->AllocateDescriptorSet(dvkSet->descriptorSets.data())) {
				return dvkSet;
			}
		}

		DVKDescriptorSetPool* setPool = new DVKDescriptorSetPool(device, 64, setLayoutsInfo, descriptorSetLayouts);
		descriptorSetPools.push_back(setPool);
		setPool->AllocateDescriptorSet(dvkSet->descriptorSets.data());

		return dvkSet;
	}

	void CShader::__compile()
	{
		__processShaderModule(vertShaderModule);
		__processShaderModule(fragShaderModule);
		__processShaderModule(geomShaderModule);
		__processShaderModule(compShaderModule);
		__processShaderModule(tescShaderModule);
		__processShaderModule(teseShaderModule);
		__generateInputInfo();
		__generateLayout();
	}

	//解析Shadercode
	void CShader::__processShaderModule(CShaderModule* shaderModule)
	{
		if (!shaderModule) {
			return;
		}

		// 保存StageInfo
		VkPipelineShaderStageCreateInfo ShaderStage;
		ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ShaderStage.stage = shaderModule->getShaderStageFlage();
		ShaderStage.module = shaderModule->getInstanceHandle();
		ShaderStage.pName = "main";
		shaderStageCreateInfos.push_back(ShaderStage);

		// 反编译Shader获取相关信息,即从SPV格式逆转换为GLSL
		spirv_cross::Compiler compiler(shaderModule->m_Data, shaderModule->m_DataSize / sizeof(uint32_t));
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		__processUniformBuffers(compiler, resources, shaderModule->m_ShaderStageFlag);
		__processTextures(compiler, resources, shaderModule->m_ShaderStageFlag);
		__processInput(compiler, resources, shaderModule->m_ShaderStageFlag);
	}

	void CShader::__processUniformBuffers(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags)
	{
		// 获取Uniform Buffer信息
		for (uint32_t i = 0; i < resources.uniform_buffers.size(); ++i)
		{
			spirv_cross::Resource& res = resources.uniform_buffers[i];
			spirv_cross::SPIRType type = compiler.get_type(res.type_id);
			spirv_cross::SPIRType base_type = compiler.get_type(res.base_type_id);
			const std::string &varName = compiler.get_name(res.id);
			const std::string &typeName = compiler.get_name(res.base_type_id);
			uint32_t uniformBufferStructSize = compiler.get_declared_struct_size(type);

			uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			// [layout (binding = 0) uniform MVPDynamicBlock] 标记为Dynamic的buffer
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorType = (typeName.find("Dynamic") != std::string::npos || dynamicUBO) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.pImmutableSamplers = nullptr;

			setLayoutsInfo.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

			// 保存UBO变量信息
			auto it = bufferParams.find(varName);
			if (it == bufferParams.end())
			{
				BufferInfo bufferInfo = {};
				bufferInfo.set = set;
				bufferInfo.binding = binding;
				bufferInfo.bufferSize = uniformBufferStructSize;
				bufferInfo.stageFlags = stageFlags;
				bufferInfo.descriptorType = setLayoutBinding.descriptorType;
				bufferParams.insert(std::make_pair(varName, bufferInfo));
			}
			else
			{
				it->second.stageFlags |= setLayoutBinding.stageFlags;
			}
		}
	}

	void CShader::__processTextures(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags)
	{
		// 获取Texture
		for (uint32_t i = 0; i < resources.sampled_images.size(); ++i)
		{
			spirv_cross::Resource& res = resources.sampled_images[i];
			spirv_cross::SPIRType type = compiler.get_type(res.type_id);
			spirv_cross::SPIRType base_type = compiler.get_type(res.base_type_id);
			const std::string&      varName = compiler.get_name(res.id);

			uint32_t set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.pImmutableSamplers = nullptr;

			setLayoutsInfo.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

			auto it = imageParams.find(varName);
			if (it == imageParams.end())
			{
				ImageInfo imageInfo = {};
				imageInfo.set = set;
				imageInfo.binding = binding;
				imageInfo.stageFlags = stageFlags;
				imageInfo.descriptorType = setLayoutBinding.descriptorType;
				imageParams.insert(std::make_pair(varName, imageInfo));
			}
			else
			{
				it->second.stageFlags |= setLayoutBinding.stageFlags = stageFlags;
			}
		}	
	}

	void CShader::__processInput(spirv_cross::Compiler& compiler, spirv_cross::ShaderResources& resources, VkShaderStageFlags stageFlags)
	{
		if (stageFlags != VK_SHADER_STAGE_VERTEX_BIT) {
			return;
		}

		// 获取input信息
		for (uint32_t i = 0; i < resources.stage_inputs.size(); ++i)
		{
			spirv_cross::Resource& res = resources.stage_inputs[i];
			spirv_cross::SPIRType type = compiler.get_type(res.type_id);
			const std::string &varName = compiler.get_name(res.id);
			uint32_t inputAttributeSize = type.vecsize;

			VertexAttribute attribute = StringToVertexAttribute(varName.c_str());
			if (attribute == VertexAttribute::VA_None)
			{
				if (inputAttributeSize == 1) {
					attribute = VertexAttribute::VA_InstanceFloat1;
				}
				else if (inputAttributeSize == 2) {
					attribute = VertexAttribute::VA_InstanceFloat2;
				}
				else if (inputAttributeSize == 3) {
					attribute = VertexAttribute::VA_InstanceFloat3;
				}
				else if (inputAttributeSize == 4) {
					attribute = VertexAttribute::VA_InstanceFloat4;
				}
			}

			// location必须连续
			uint32_t location = compiler.get_decoration(res.id, spv::DecorationLocation);
			DVKAttribute dvkAttribute = {};
			dvkAttribute.location = location;
			dvkAttribute.attribute = attribute;
			m_InputAttributes.push_back(dvkAttribute);
		}
	}

	void CShader::__generateInputInfo()
	{
		// 对inputAttributes进行排序，获取Attributes列表
		std::sort(m_InputAttributes.begin(), m_InputAttributes.end(), [](const SAttribute& a, const SAttribute& b) -> bool {
			return a.location < b.location;
		});

		// 对inputAttributes进行归类整理
		for (uint32_t i = 0; i < m_InputAttributes.size(); ++i)
		{
			VertexAttribute attribute = m_InputAttributes[i].attribute;
			perVertexAttributes.push_back(attribute);
		}

		// 生成Bindinfo
		inputBindings.resize(0);
		if (perVertexAttributes.size() > 0)
		{
			uint32_t stride = 0;
			for (uint32_t i = 0; i < perVertexAttributes.size(); ++i) {
				stride += VertexAttributeToSize(perVertexAttributes[i]);
			}
			VkVertexInputBindingDescription perVertexInputBinding = {};
			perVertexInputBinding.binding = 0;
			perVertexInputBinding.stride = stride;
			perVertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			inputBindings.push_back(perVertexInputBinding);
		}

		// 生成attributes info
		int location = 0;
		if (perVertexAttributes.size() > 0)
		{
			uint32_t offset = 0;
			for (uint32_t i = 0; i < perVertexAttributes.size(); ++i)
			{
				VkVertexInputAttributeDescription inputAttribute = {};
				inputAttribute.binding = 0;
				inputAttribute.location = location;
				inputAttribute.format = VertexAttributeToVkFormat(perVertexAttributes[i]);
				inputAttribute.offset = offset;
				offset += VertexAttributeToSize(perVertexAttributes[i]);
				inputAttributes.push_back(inputAttribute);

				location += 1;
			}
		}
	}

	void CShader::__generateLayout()
	{
		std::vector<CDescriptorSetLayoutInfo>& setLayouts = setLayoutsInfo.setLayouts;

		// 先按照set进行排序
		std::sort(setLayouts.begin(), setLayouts.end(), [](const CDescriptorSetLayoutInfo& a, const CDescriptorSetLayoutInfo& b) -> bool {
			return a.set < b.set;
		});

		// 再按照binding进行排序
		for (uint32_t i = 0; i < setLayouts.size(); ++i)
		{
			std::vector<VkDescriptorSetLayoutBinding>& bindings = setLayouts[i].bindings;
			std::sort(bindings.begin(), bindings.end(), [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) -> bool {
				return a.binding < b.binding;
			});
		}

		for (uint32_t i = 0; i < setLayoutsInfo.setLayouts.size(); ++i)
		{
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			CDescriptorSetLayoutInfo& setLayoutInfo = setLayoutsInfo.setLayouts[i];

			VkDescriptorSetLayoutCreateInfo descSetLayoutInfo;
			descSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descSetLayoutInfo.bindingCount = setLayoutInfo.bindings.size();
			descSetLayoutInfo.pBindings = setLayoutInfo.bindings.data();
		    vkCreateDescriptorSetLayout(device, &descSetLayoutInfo, nullptr, &descriptorSetLayout);

			descriptorSetLayouts.push_back(descriptorSetLayout);
		}

		VkPipelineLayoutCreateInfo pipeLayoutInfo;
		pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
		pipeLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		vkCreatePipelineLayout(device, &pipeLayoutInfo, nullptr, &pipelineLayout);
	}

	CDescriptorSetPool::CDescriptorSetPool(VkDevice inDevice, uint32_t inMaxSet, const CDescriptorSetLayoutsInfo& setLayoutsInfo, const std::vector<VkDescriptorSetLayout>& inDescriptorSetLayouts)
	{
		device = inDevice;
		maxSet = inMaxSet;
		usedSet = 0;
		descriptorSetLayouts = inDescriptorSetLayouts;

		std::vector<VkDescriptorPoolSize> poolSizes;
		for (uint32_t i = 0; i < setLayoutsInfo.setLayouts.size(); ++i)
		{
			const CDescriptorSetLayoutInfo& setLayoutInfo = setLayoutsInfo.setLayouts[i];
			for (uint32_t j = 0; j < setLayoutInfo.bindings.size(); ++j)
			{
				VkDescriptorPoolSize poolSize = {};
				poolSize.type = setLayoutInfo.bindings[j].descriptorType;
				poolSize.descriptorCount = setLayoutInfo.bindings[j].descriptorCount;
				poolSizes.push_back(poolSize);
			}
		}

		VkDescriptorPoolCreateInfo descriptorPoolInfo;
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = poolSizes.size();
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSet;
		vkCreateDescriptorPool(inDevice, &descriptorPoolInfo, nullptr, &descriptorPool);
	}

	CDescriptorSetPool::~CDescriptorSetPool()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}
	}

	bool CDescriptorSetPool::allocateDescriptorSet(VkDescriptorSet* descriptorSet)
	{
		if (usedSet + descriptorSetLayouts.size() >= maxSet) {
			return false;
		}

		usedSet += descriptorSetLayouts.size();
		VkDescriptorSetAllocateInfo AllocateInfo = {};
		AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocateInfo.descriptorPool = descriptorPool;
		AllocateInfo.descriptorSetCount = descriptorSetLayouts.size();
		AllocateInfo.pSetLayouts = descriptorSetLayouts.data();

		//不需要明确清理描述符集合，因为它们会在描述符对象池销毁的时候自动清
		////从描述符池分配描述符集对象
		if (vkAllocateDescriptorSets(device, &AllocateInfo, descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor set!");
		}
		return  true;
	}

	void CDescriptorSet::writeBuffer(const std::string& name, const VkDescriptorBufferInfo* bufferInfo)
	{
		auto it = setLayoutsInfo.paramsMap.find(name);
		if (it == setLayoutsInfo.paramsMap.end())
		{
			throw std::runtime_error("Failed write buffer, the name not found!");
			return;
		}

		auto bindInfo = it->second;

		VkWriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSets[bindInfo.set];
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = setLayoutsInfo.GetDescriptorType(bindInfo.set, bindInfo.binding);
		writeDescriptorSet.pBufferInfo = bufferInfo;
		writeDescriptorSet.dstBinding = bindInfo.binding;
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}

	void CDescriptorSet::writeBuffer(const std::string& name, const CBuffer* buffer)
	{
		auto it = setLayoutsInfo.paramsMap.find(name);
		if (it == setLayoutsInfo.paramsMap.end())
		{
			throw std::runtime_error("Failed write buffer, the name not found!");
			return;
		}

		auto bindInfo = it->second;

		VkWriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSets[bindInfo.set];
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = setLayoutsInfo.GetDescriptorType(bindInfo.set, bindInfo.binding);
		writeDescriptorSet.pBufferInfo = &(buffer->getDescriptorBufferInfo());
		writeDescriptorSet.dstBinding = bindInfo.binding;
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}

	void CDescriptorSet::writeImage(const std::string& name, DVKTexture* texture)
	{
		auto it = setLayoutsInfo.paramsMap.find(name);
		if (it == setLayoutsInfo.paramsMap.end())
		{
			throw std::runtime_error("Failed write buffer, the name not found!");
			return;
		}

		auto bindInfo = it->second;

		VkWriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSets[bindInfo.set];
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = setLayoutsInfo.GetDescriptorType(bindInfo.set, bindInfo.binding);
		writeDescriptorSet.pBufferInfo = nullptr;
		writeDescriptorSet.pImageInfo = &(texture->descriptorInfo);
		writeDescriptorSet.dstBinding = bindInfo.binding;
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}


	VkDescriptorType CDescriptorSetLayoutsInfo::GetDescriptorType(uint32_t set, uint32_t binding)
	{
		for (uint32_t i = 0; i < setLayouts.size(); ++i)
		{
			if (setLayouts[i].getSet() == set)
			{
				for (uint32_t j = 0; j < (setLayouts[i].getBinding()).size(); ++j)
				{
					if ((setLayouts[i].getBinding())[j].binding == binding)
					{
						return (setLayouts[i].getBinding())[j].descriptorType;
					}
				}
			}
		}

		return VK_DESCRIPTOR_TYPE_END_RANGE;
	}

	void CDescriptorSetLayoutsInfo::AddDescriptorSetLayoutBinding(const std::string& varName, uint32_t set, VkDescriptorSetLayoutBinding binding)
	{
		CDescriptorSetLayoutInfo* setLayout = nullptr;

		for (uint32_t i = 0; i < setLayouts.size(); ++i)
		{
			if (setLayouts[i].getSet() == set)
			{
				setLayout = &(setLayouts[i]);
				break;
			}
		}

		if (setLayout == nullptr)
		{
			setLayouts.push_back({ });
			setLayout = &(setLayouts[setLayouts.size() - 1]);
		}

		for (uint32_t i = 0; i < setLayout->getBinding().size(); ++i)
		{
			VkDescriptorSetLayoutBinding& setBinding = setLayout->getBinding()[i];
			if (setBinding.binding == binding.binding && setBinding.descriptorType == binding.descriptorType)
			{
				setBinding.stageFlags = setBinding.stageFlags | binding.stageFlags;
				return;
			}
		}

		setLayout ->set = set;
		setLayout->bindings.push_back(binding);

		// 保存变量映射信息
		BindInfo paramInfo = {};
		paramInfo.set = set;
		paramInfo.binding = binding.binding;
		paramsMap.insert(std::make_pair(varName, paramInfo));
	}
}