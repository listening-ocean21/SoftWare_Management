#include "../../Head/1_Device/CLogicalDevice.h"

class CLogicalDevice;

namespace vk_Demo
{
	//Fence
	class CFence
	{
	public:
		enum class State
		{
			UNSIGNALED,
			SIGNALED
		};
		CFence(CLogicalDevice* vDevcie, bool vSignaled);

		//�����κδ�����FenceManageͳһ����
		virtual ~CFence() {};

		inline VkFence getInstaceHandle() const { return m_Fence; }
		inline bool isFenceSignaled() const {return  m_State == State::SIGNALED;}
		inline State getFenceState() const { return m_State; };
		inline void  setFenceState(State vState) { m_State = vState; };

		friend class CFenceManager;
	private:
		VkFence m_Fence;
		State   m_State;
	};

	//Fence����أ������ѽ�����Fence,���½���Fence,����Ҫ����������Դ
	class CFenceManager
	{
	public:
		CFenceManager() {};
		virtual ~CFenceManager();

		CFence* createFence(bool vCreateSignaled = false);
		bool waitForFence(CFence* cFence, uint64_t VTimeInNanoseconds);
		void resetFence(CFence* cFence);
		void releaseFence(CFence* cFence);
		void waitAndReleaseFence(CFence* cFence, uint64_t timeInNanoseconds);
		bool checkFenceState(CFence* vFence);

	private:
		CLogicalDevice*      m_Device;
		std::vector<CFence*> m_FreeFences;
		std::vector<CFence*> m_UsedFences;

		void __destoryFence(CFence* vFence);
	};
	class CSemaphore
	{
	public:
		CSemaphore(CLogicalDevice* vDdevice);

		virtual ~CSemaphore();
		inline VkSemaphore getHandle() const{ return m_Semaphore;}
	private:
		VkSemaphore       m_Semaphore;
		CLogicalDevice*   m_Device;
	};
}