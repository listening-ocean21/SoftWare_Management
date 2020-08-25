#pragma once
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Class��define  the class of FPS Camera
class CCamera {
public:
	//����������
	CCamera(glm::vec3 vPosition, glm::vec3 vTarget, glm::vec3 vWorldUp);
	//ŷ���ǹ���
	CCamera(glm::vec3 vPosition, float vPitch, float vYaw, glm::vec3 vWorldUp);

	glm::mat4 getViewMatrix() const;
	void setMouseSpeedX(float vMouseSpeedX) { m_MouseSpeedX = vMouseSpeedX; };
	void setMouseSpeedY(float vMouseSpeedY) { m_MouseSpeedY = vMouseSpeedY; };
	void setKeyBoardSpeedX(float vKeyBoardSpeedX) { m_KeyBoardSpeedX = vKeyBoardSpeedX; };
	void setKeyBoardSpeedY(float vKeyBoardSpeedX) { m_KeyBoardSpeedY = vKeyBoardSpeedX; };
	void setKeyBoardSpeedZ(float vKeyBoardSpeedX) { m_KeyBoardSpeedZ = vKeyBoardSpeedX; };
	float getKeyBoardSpeedX() const { return m_KeyBoardSpeedX; };
	float getKeyBoardSpeedY() const { return m_KeyBoardSpeedY; };
	float getKeyBoardSpeedZ() const { return m_KeyBoardSpeedZ; };
	glm::vec3 getCameraPos()  const { return m_Position; };
	glm::vec3 getCameraDir()  const { return m_Forward; };

	//����ƶ�
	void processMouseMovement(float vDeltaX, float vDeltaY);
	//�����������λ��
	void updateCameraPos();

private: 
	glm::vec3 m_Position;   //camera's Postion
	glm::vec3 m_Forward;    //camera's direction
	glm::vec3 m_Right;      //�����������
	glm::vec3 m_Up;         //�����������
	glm::vec3 m_WorldUp;    //��������ϵ������

	float m_Pitch;      //������
	float  m_Yaw;        //ƫ����

	float m_MouseSpeedX = 0.001f;//����ƶ�X����
	float m_MouseSpeedY = 0.001f;//����ƶ�Y����
	float m_KeyBoardSpeedX = 0.0f;//����X�ƶ�����
	float m_KeyBoardSpeedY = 0.0f;//����Y�ƶ�����
	float m_KeyBoardSpeedZ = 0.0f;//����Z�ƶ�����

	void __updateCameraVector();
};

