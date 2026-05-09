#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>
#include "CTask.h"

using namespace std;

class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(size_t nThreads);
	~ThreadPool();
public:
	//初始化线程池
	void InitThreadPool(size_t nThreads);
	//线程运行函数
	void Run();
	void Stop();
	void AddTask(std::shared_ptr<CTask > spTask);
	int GetCurrentThreadId();
private:
	//线程池
	std::vector<std::thread > m_workerThreads;
	//任务队列
	std::queue<std::shared_ptr<CTask > > m_spTaskQueue;
	//线程取任务同步互斥量
	std::mutex m_TaskQueue_Mutex;
	//线程执行任务的条件允许变量
	std::condition_variable	m_task_cv;
	//停止信号量
	bool m_bStop;
};

