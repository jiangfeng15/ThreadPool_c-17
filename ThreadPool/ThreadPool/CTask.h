#pragma once
#include <string>
using namespace std;

class CTask
{
public:
	CTask();
	~CTask();
	void SetTaskName(char* taskname);
	//指定Run接口
	virtual void Run(/*void *pvargs*/) = 0;
	int GetTaskNo(void);
	void SetTaskNo(int taskno);
	char* GetTaskName(void) const;

private:
	int m_taskNo;                    //任务编号
	string m_taskName;               //任务名称
};

