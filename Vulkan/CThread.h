#pragma once
#include <thread>
#include <queue>
#include <mutex>

class Thread
{
private:
	bool destroying = false;
	std::thread worker;
	std::queue<std::function<void()>> jobQueue;
	std::mutex queueMutex;
	std::condition_variable condition; //条件变量

	// Loop through all remaining jobs 循环遍历
	void queueLoop()
	{
		while (true)
		{
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
				if (destroying)
				{
					break;
				}
				//取队列第一个任务
				job = jobQueue.front();
			}

			job();

			{
				std::lock_guard<std::mutex> lock(queueMutex);
				jobQueue.pop(); //弹出第一个队首值，不返回值
				condition.notify_one();  //随机唤醒一个进程
			}
		}
	}

public:
	Thread()
	{
		worker = std::thread(&Thread::queueLoop, this);
	}

	~Thread()
	{
		if (worker.joinable())
		{
			wait();
			queueMutex.lock();
			destroying = true;
			condition.notify_one();  //notify_one()（随机唤醒一个等待的线程）和notify_all()（唤醒所有等待的线程
			queueMutex.unlock();
			worker.join();
		}
	}

	// Add a new job to the thread's queue
	void addJob(std::function<void()> function)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		jobQueue.push(std::move(function));
		condition.notify_one();
	}

	// Wait until all work items have been finished
	void wait()
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		condition.wait(lock, [this]() { return jobQueue.empty(); });
	}
};

class ThreadPool
{
public:
	std::vector<std::unique_ptr<Thread>> threads;

	// Sets the number of threads to be allocted in this pool
	void setThreadCount(uint32_t count)
	{
		threads.clear();
		for (auto i = 0; i < count; i++)
		{
			threads.push_back(std::make_unique<Thread>());
		}
	}

	// Wait until all threads have finished their work items
	void wait()
	{
		for (auto &thread : threads)
		{
			thread->wait();
		}
	}
};

