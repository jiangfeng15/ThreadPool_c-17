#include "pch.h"
#include "ThreadPool.h"


ThreadPool::ThreadPool()
	:m_bStop(false)
{
}

ThreadPool::ThreadPool(size_t nThreads)
	: m_bStop(false)
{
	string strLog;
	for (size_t i = 0; i < nThreads; i++)
	{
		m_workerThreads.push_back(std::thread(&ThreadPool::Run, this));
	}
	for (std::thread &worker : m_workerThreads)
	{
		std::thread::id threadid = worker.get_id();

		std::ostringstream oss;
		oss << threadid;
		std::string stid = oss.str();

		int m_ThreadID = std::stoull(stid);

		strLog.clear();
		strLog = strLog + "线程池创建子线程 [ " + std::to_string(m_ThreadID) + " ]";
		std::cout << strLog << std::endl;
	}
}

ThreadPool::~ThreadPool()
{
	if (!m_bStop)
	{
		std::cout << "调用析构函数" << std::endl;
		{
			std::unique_lock<std::mutex> lock(m_TaskQueue_Mutex);
			m_bStop = true;
		}
		m_task_cv.notify_all();
		for (std::thread &worker : m_workerThreads)
		{
			if(worker.joinable())
				worker.join();
		}
		std::cout << "调用析构函数结束" << std::endl;
	}
}

void ThreadPool::InitThreadPool(size_t nThreads)
{
	string strLog;
	for (size_t i = 0; i < nThreads; i++)
	{
		m_workerThreads.push_back(std::thread(&ThreadPool::Run, this));
	}
	for (std::thread &worker : m_workerThreads)
	{
		std::thread::id threadid = worker.get_id();

		std::ostringstream oss;
		oss << threadid;
		std::string stid = oss.str();

		int m_ThreadID = std::stoull(stid);

		strLog.clear();
		strLog = strLog + "线程池创建子线程 [ " + std::to_string(m_ThreadID) + " ]";
		std::cout << strLog << std::endl;
	}
}

void ThreadPool::Run()
{
	for (;;)//线程在执行
	{
		//等待取任务阻塞线程，知道接收到停止信号量，或者任务队列不为空时停止阻塞线程
		std::unique_lock<std::mutex> lock(this->m_TaskQueue_Mutex);
		this->m_task_cv.wait(lock,
			[this] { return this->m_bStop || !this->m_spTaskQueue.empty(); }); //|| !this->m_spTaskQueue.empty()
		if (this->m_bStop)
			break;
		//取任务
		shared_ptr<CTask > wpTask = m_spTaskQueue.front();
		//任务队列移除该任务
		m_spTaskQueue.pop();
		//执行任务
		std::thread::id tId = std::this_thread::get_id();

		std::ostringstream oss;
		oss << tId;
		std::string stid = oss.str();

		int nThreadId = std::stoull(stid);
		std::cout << "线程 [" << GetCurrentThreadId() << "]执行任务" << std::endl;
		wpTask->Run();
	}
	std::cout << "线程 [" << GetCurrentThreadId() << "]执行任务结束" << std::endl;
}

void ThreadPool::Stop()
{
	{
		std::unique_lock<std::mutex> lock(m_TaskQueue_Mutex);
	}
	m_task_cv.notify_all();
	m_bStop = true;
	for (std::thread &worker : m_workerThreads)
		worker.join();
}
void ThreadPool::AddTask(std::shared_ptr<CTask > spTask)
{
	if (!m_bStop)//线程未结束
	{
		std::unique_lock<std::mutex> lk(m_TaskQueue_Mutex);
		m_spTaskQueue.emplace(spTask);
		m_task_cv.notify_one();
	}
}

int ThreadPool::GetCurrentThreadId()
{
	std::thread::id tId= std::this_thread::get_id();

	std::ostringstream oss;
	oss << tId;
	std::string stid = oss.str();

	int nThreadId = std::stoull(stid);

	return nThreadId;
}