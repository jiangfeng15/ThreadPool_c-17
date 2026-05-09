// ThreadPool.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include<windows.h> 
#include "CTask.h"
#include "ThreadPool.h"

class CWorkTask : public CTask
{
public:
	CWorkTask()
	{
		i = 0;
	}

	~CWorkTask()
	{
	}

	void Run()
	{
		std::string strLog = "";
		strLog += "Task No.[ " + std::to_string(GetTaskNo()) + " ]" + "Task name [ " + GetTaskName() + " ]";
		std::cout << strLog << endl;
	}
public:
	int i;
};

class CWorkTask2 : public CTask
{
public:
	CWorkTask2()
	{
		i = 0;
	}

	~CWorkTask2()
	{
	}

	void Run()
	{
		std::string strLog = "";
		strLog += "Task No.[ " + std::to_string(GetTaskNo()) + " ]" + "Task name [ " + GetTaskName() + " ]";
		std::cout << strLog << endl;
	}
public:
	int i;
};

int main()
{
	ThreadPool pool(10);
	for (int i = 0; i < 10000; i++)
	{

		char cTaskName[50] = { 0x00 };
		CWorkTask * pTask = new CWorkTask();
		pTask->i = i + 1;
		pTask->SetTaskNo(i + 1);
		std::snprintf(cTaskName, sizeof(cTaskName), "task%03d", i + 1);
		pTask->SetTaskName(cTaskName);
		shared_ptr<CWorkTask > spTask;
		spTask.reset(pTask);
		pool.AddTask(spTask);
		//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	char cTaskName[50] = { 0x00 };
	CWorkTask * pTask = new CWorkTask();
	pTask->i = 999;
	pTask->SetTaskNo(999);
	std::snprintf(cTaskName, sizeof(cTaskName), "工作任务%03d", 999);
	pTask->SetTaskName(cTaskName);
	shared_ptr<CWorkTask > spTask;
	spTask.reset(pTask);
	pool.AddTask(spTask);

	//pool.Stop();

	while (1)
	{
		Sleep(10);
	}

    std::cout << "Hello World!\n"; 
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
