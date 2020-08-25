#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D TexSampler;
layout(binding = 2) uniform sampler2D DepthShadowMap;

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in vec3 FragBaseLight;
layout(location = 4) in vec3 ViewPos;
layout(location = 5) in vec3 LightDirection;

layout(location = 6) in vec4 ShadowCoord;
layout(location = 7) in float Shininess;

layout(location = 0) out vec4 FragColor;

vec3 LightAmbient = vec3(0.1f, 0.1f, 0.1f);
vec3 LightDiffUse = vec3(0.5f, 0.5f, 0.5f);
vec3 LightSpecular = vec3(0.8f, 0.8f, 0.8f);

float textureSampler(vec4 vFragLightSpacePos)
{
	float Shadow = 1.0f;
	//把光空间片段位置转换为裁切空间的标准化设备坐标。
	vec3 ShadowCoord = vFragLightSpacePos.xyz / vFragLightSpacePos.w;
	//将深度变化到0-1之间
	ShadowCoord = ShadowCoord * 0.5 + 0.5;
	float CurrentDepth = ShadowCoord.z;
	float DepthMapZ = texture(DepthShadowMap, ShadowCoord.xy).r;
	Shadow = CurrentDepth < DepthMapZ ? 0.0 : 1.0;
	return Shadow;
}
void main()
{
	vec3 Normal = normalize(FragNormal);
	vec3 LightDir = normalize(-LightDirection);

	//漫反射
	float Diff = max(dot(Normal, LightDir), 0);
	//镜面反射
	vec3 ReflectDir = reflect(-LightDir, Normal);
	float Spec = pow(max(dot(normalize(ViewPos - FragPos), ReflectDir), 0), Shininess);

	//合并结果
	vec3 Ambient = LightAmbient * vec3(texture(TexSampler, FragTexCoord));
	vec3 Diffuse = LightDiffUse * Diff * vec3(texture(TexSampler, FragTexCoord));
	vec3 Specular = LightSpecular * Spec * vec3(texture(TexSampler, FragTexCoord));

	//计算阴影	
	float Shadow = textureSampler(ShadowCoord);
	FragColor = vec4((Ambient + (1.0 - Shadow) * (Diffuse + Specular)) * FragBaseLight, 1.0f);
}