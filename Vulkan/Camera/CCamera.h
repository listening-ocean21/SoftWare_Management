#pragma once
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Class：define  the class of FPS Camera
class CCamera {
public:
	//三向量构造
	CCamera(glm::vec3 vPosition, glm::vec3 vTarget, glm::vec3 vWorldUp);
	//欧拉角构造
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

	//鼠标移动
	void processMouseMovement(float vDeltaX, float vDeltaY);
	//更新摄像机的位置
	void updateCameraPos();

private: 
	glm::vec3 m_Position;   //camera's Postion
	glm::vec3 m_Forward;    //camera's direction
	glm::vec3 m_Right;      //相机的右向量
	glm::vec3 m_Up;         //相机的上向量
	glm::vec3 m_WorldUp;    //世界坐标系上向量

	float m_Pitch;      //俯仰角
	float  m_Yaw;        //偏航角

	float m_MouseSpeedX = 0.001f;//鼠标移动X速率
	float m_MouseSpeedY = 0.001f;//鼠标移动Y速率
	float m_KeyBoardSpeedX = 0.0f;//键盘X移动速率
	float m_KeyBoardSpeedY = 0.0f;//键盘Y移动速率
	float m_KeyBoardSpeedZ = 0.0f;//键盘Z移动速率

	void __updateCameraVector();
};

