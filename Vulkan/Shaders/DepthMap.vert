#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform UBObject
{
	mat4 LightSpaceVP; //��Դת������ ����������ϵת���ü�����ϵ
	mat4 ModelMatrix;
}UBO;

layout(location = 0) in vec3 aPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
	gl_Position = UBO.LightSpaceVP * UBO.ModelMatrix * vec4(aPos, 1.0f);
}