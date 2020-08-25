#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform  UniformBufferObject {
	mat4 ModelMatrix;
	mat4 ViewMatrix;
	mat4 ProjectiveMatrix;
	mat4 LightSpaceMatrix; //光源转化矩阵， 将世界坐标系转到裁剪坐标系

	vec3 BaseLight;
	vec3 LightPos;
	vec3 LightDirection;
	vec3 ViewPos;

	float AmbientStrenght;
	float SpecularStrenght;

	vec3 FlashPos;
	vec3 FlashDir;
	float OuterCutOff;
	float InnerCutOff;
}UBO;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aAmbient;
layout(location = 5) in vec3 aDiffuse;
layout(location = 6) in vec3 aSpecular;
layout(location = 7) in float aShininess;


layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 FragNormal;
layout(location = 2) out vec2 FragTexCoord;
layout(location = 3) out vec3 FragBaseLight;
layout(location = 4) out vec3 ViewPos;
layout(location = 5) out vec3 LightDirection;

layout(location = 6) out vec4 ShadowCoord;
layout(location = 7) out float Shininess;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
	gl_Position = UBO.ProjectiveMatrix * UBO.ViewMatrix * UBO.ModelMatrix * vec4(aPos, 1.0f);

	//转换到世界坐标系下
	FragPos = vec3(UBO.ModelMatrix * vec4(aPos, 1.0f));
	FragNormal = mat3(transpose(inverse(UBO.ModelMatrix))) * aNormal;
	FragTexCoord = aTexCoord;
	FragBaseLight = UBO.BaseLight;
	ViewPos = UBO.ViewPos;
	LightDirection = UBO.LightDirection;
	Shininess = aShininess;
	ShadowCoord = UBO.LightSpaceMatrix * vec4(FragPos, 1.0f);
}
