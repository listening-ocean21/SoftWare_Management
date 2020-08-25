#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	mat4 lightSpace;
	vec3 baseLight;
	float ambientStrength;
	vec3 lightPos;
	float specularStrength ;
	vec3 viewPos;
	vec3 lightDirect;
	vec3 flashColor;
	vec3 flashPos;
	float outerCutOff;
	vec3 flashDir;
	float flashCutOff ;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inM_ambient;
layout(location = 5) in vec3 inM_diffuse;
layout(location = 6) in vec3 inM_specular;
layout(location = 7) in float inM_shininess;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragBaseLight;
layout(location = 4) out float ambientStrength;
layout(location = 5) out vec3 lightPos;
layout(location = 6) out vec3 fragPos;
layout(location = 7) out float specularStrength;
layout(location = 8) out vec3 viewPos;
//Material
layout(location = 9) out vec3 m_ambient;
layout(location = 10) out vec3 m_diffuse;
layout(location = 11) out vec3 m_specular;
layout(location = 12) out float m_shininess;
layout(location = 13) out vec3 lightDirect;
layout(location = 14) out vec3 flashPos;
layout(location = 15) out vec3 flashDir;
layout(location = 16) out float flashCutOff;
layout(location = 17) out float outerCutOff;
layout(location = 18) out vec3 flashColor;
layout(location = 19) out vec4 outShadowCoord;

layout(push_constant) uniform PushConsts {
	vec3 objPos;
} pushConsts;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition + pushConsts.objPos, 1.0);
	//fragPos =vec3( ubo.model * vec4(inPosition+ pushConsts.objPos, 1.0));
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
	fragPos =vec3( ubo.model * vec4(inPosition, 1.0));
    fragColor = inColor;
    fragNormal = mat3(transpose(inverse(ubo.model))) *inNormal;
	fragTexCoord = inTexCoord;
	fragBaseLight = ubo.baseLight;
	ambientStrength= ubo.ambientStrength;
	lightPos=ubo.lightPos;
	specularStrength=ubo.specularStrength;
	viewPos=ubo.viewPos;
	m_ambient = inM_ambient;
	m_diffuse = inM_diffuse;
	m_specular = inM_specular;
	m_shininess = inM_shininess;
	lightDirect= ubo.lightDirect;
	flashPos= ubo.flashPos;
	flashDir= ubo.flashDir;
	flashCutOff= ubo.flashCutOff;
	outerCutOff= ubo.outerCutOff;
	flashColor= ubo.flashColor;
	outShadowCoord =  ( biasMat * ubo.lightSpace * ubo.model ) * vec4(inPosition, 1.0);	
}
