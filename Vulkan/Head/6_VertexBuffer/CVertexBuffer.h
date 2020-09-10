#include "../../Head/1_Device/CLogicalDevice.h"
#include "../../Head/2_Buffer/CBuffer.h"
#include "../../Head/3_CommandBuffer/CCommandBuffer.h"

namespace vk_Demo
{
	class CVertexBuffer
	{
	public:
		enum EVertexAttribute
		{
			VA_None = 0,
			VA_Position,
			VA_UV0,
			VA_UV1,
			VA_Normal,
			VA_Tangent,
			VA_Color,
			VA_SkinWeight,
			VA_SkinIndex,
			VA_Custom0,
			VA_Custom1,
			VA_Custom2,
			VA_Custom3,
			VA_Count,
		};

		CVertexBuffer();
		~CVertexBuffer();

		CVertexBuffer* createVertexBuffer(std::shared_ptr<CLogicalDevice> vulkanDevice, CCommandBuffer* cmdBuffer, std::vector<float> vertices, const std::vector<EVertexAttribute>& attributes);

		VkVertexInputBindingDescription getInputBinding();
		std::vector<VkVertexInputAttributeDescription> getInputAttributes(const std::vector<EVertexAttribute>& shaderInputs);
		void bind(VkCommandBuffer cmdBuffer);
	private:
		VkDevice                         m_Device = VK_NULL_HANDLE;
		std::vector<EVertexAttribute>    m_Attributes;
		CBuffer*                         m_VertexBuffer = nullptr;
		VkDeviceSize					 m_Offset = 0;
		inline uint32_t __vertexAttributeToSize(EVertexAttribute vAttribute)
		{
			if (vAttribute == EVertexAttribute::VA_Position) {
				return 3 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_UV0) {
				return 2 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_UV1) {
				return 2 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_Normal) {
				return 3 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_Tangent) {
				return 4 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_Color) {
				return 3 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_SkinWeight) {
				return 4 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_SkinIndex) {
				return 4 * sizeof(float);
			}
			else if (vAttribute == EVertexAttribute::VA_Custom0 ||
				vAttribute == EVertexAttribute::VA_Custom1 ||
				vAttribute == EVertexAttribute::VA_Custom2 ||
				vAttribute == EVertexAttribute::VA_Custom3
				)
			{
				return 4 * sizeof(float);
			}
			return 0;
		}

		inline VkFormat __vertexAttributeToVkFormat(EVertexAttribute attribute)
		{
			VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;
			if (attribute == EVertexAttribute::VA_Position) {
				format = VK_FORMAT_R32G32B32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_UV0) {
				format = VK_FORMAT_R32G32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_UV1) {
				format = VK_FORMAT_R32G32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_Normal) {
				format = VK_FORMAT_R32G32B32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_Tangent) {
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_Color) {
				format = VK_FORMAT_R32G32B32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_SkinWeight) {
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_SkinIndex) {
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			else if (attribute == EVertexAttribute::VA_Custom0 ||
				attribute == EVertexAttribute::VA_Custom1 ||
				attribute == EVertexAttribute::VA_Custom2 ||
				attribute == EVertexAttribute::VA_Custom3
				)
			{
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			return format;
		}
	};
}