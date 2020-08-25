 #version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragBaseLight;
layout(location = 4) in float ambientStrength;
layout(location = 5) in vec3 lightPos;
layout(location = 6) in vec3 fragPos;
layout(location = 7) in float specularStrength;
layout(location = 8) in vec3 viewPos;
//Material
layout(location = 9) in vec3 m_ambient;
layout(location = 10) in vec3 m_diffuse;
layout(location = 11) in vec3 m_specular;
layout(location = 12) in float m_shininess;
layout(location = 13) in vec3 lightDirect;
layout(location = 14) in vec3 flashPos;
layout(location = 15) in vec3 flashDir;
layout(location = 16) in float flashCutOff;
layout(location = 17) in float outerCutOff;
layout(location = 18) in vec3 flashColor;
layout(location = 19) in vec4 inShadowCoord;

layout(location = 0) out vec4  FragColor;

vec3 lightAmbient  = vec3( 0.1f, 0.1f, 0.1f);
vec3 lightDiffuse  = vec3( 0.5f, 0.5f, 0.5f);
vec3 lightSpecular = vec3( 0.8f, 0.8f, 0.8f);
float constant = 1.0f;
float linear =  0.09f;
float quadratic = 0.032f;
float distance = length(lightPos - fragPos);
float attenuation = 1.0 / (constant + linear * distance +  quadratic * (distance * distance));

float textureProj(vec4 P, vec2 off)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if(shadowCoord.w > 0.0){
		shadow = 2;
		}
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.3;
		}
	}
	return shadow;
}

void main() {

	vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-lightDirect);
    // 漫反射着色
    float diff =  max(dot(norm, lightDir), 0.0);
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(normalize(viewPos - fragPos), reflectDir), 0.0), m_shininess);
    // 合并结果
    vec3 ambient  = lightAmbient  * vec3(texture(texSampler, fragTexCoord));
    vec3 diffuse  = lightDiffuse  * diff * vec3(texture(texSampler, fragTexCoord));
    vec3 specular = lightSpecular * spec * vec3(texture(texSampler, fragTexCoord));

	// 计算阴影
	float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
    vec3 color = (ambient +  shadow * (diffuse + specular)) * fragBaseLight;
	FragColor = vec4(color * shadow, 1.0);

}
