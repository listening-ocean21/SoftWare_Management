#include "./Camera/CCamera.h"

//三向量构造
CCamera::CCamera(glm::vec3 vPosition, glm::vec3 vTarget, glm::vec3 vWorldUp) 
{
	m_Position = vPosition;
	m_WorldUp  = vWorldUp;
	//摄像机方向与实际指向相反
	m_Forward  = glm::normalize(vPosition - vTarget);

	m_Right = glm::normalize(glm::cross(m_WorldUp, m_Forward));
	m_Up = glm::normalize(glm::cross(m_Forward, m_Right));
}
//欧拉角构造
CCamera::CCamera(glm::vec3 vPosition, float vPitch, float vYaw, glm::vec3 vWorldUp)
{
	m_Position = vPosition;
	m_WorldUp  = vWorldUp;
	m_Pitch = vPitch;
	m_Yaw = vYaw;

	m_Forward.x = glm::cos(vPitch) * glm::sin(vYaw);
	m_Forward.y = glm::sin(vPitch);
	m_Forward.z = glm::cos(vPitch) * glm::cos(vYaw);


	m_Right = glm::normalize(glm::cross(m_WorldUp, m_Forward));
	m_Up = glm::normalize(glm::cross(m_Forward, m_Right));
}

glm::mat4 CCamera::getViewMatrix() const
{
	return glm::lookAt(m_Position, m_Position + m_Forward, m_WorldUp);
}

void CCamera::updateCameraPos()
{
	m_Position += m_Forward * m_KeyBoardSpeedZ * 0.01f + m_Right * m_KeyBoardSpeedX * 0.01f + m_Up * m_KeyBoardSpeedY  * 0.01f;
}
void CCamera::processMouseMovement(float vDeltaX, float vDeltaY)
{
	m_Pitch += vDeltaY * m_MouseSpeedY;
	m_Yaw += vDeltaX * m_MouseSpeedX;

	if (m_Pitch > 89.0f)
	{
		m_Pitch = 89.0f;
	}
	if (m_Pitch < -89.0f)
	{
		m_Pitch = -89.0f;
	}

	//更新摄像机角度
	__updateCameraVector();
}

void CCamera::__updateCameraVector()
{
	m_Forward.x = glm::cos(m_Pitch) * glm::sin(m_Yaw);
	m_Forward.y = glm::sin(m_Pitch);
	m_Forward.z = glm::cos(m_Pitch) * glm::cos(m_Yaw);


	m_Right = glm::normalize(glm::cross(m_WorldUp, m_Forward));
	m_Up = glm::normalize(glm::cross(m_Forward, m_Right));
}