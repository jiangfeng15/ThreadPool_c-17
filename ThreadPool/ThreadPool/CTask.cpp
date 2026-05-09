#include "pch.h"
#include "CTask.h"


CTask::CTask():/*m_pWorkThread(NULL),*/
	m_taskNo(0),
	m_taskName("")
{
}


CTask::~CTask()
{
}

void CTask::SetTaskName(char* taskname)
{
	m_taskName = taskname;
}
//设置或获取任务编号
int CTask::GetTaskNo(void)
{
	return m_taskNo;
}

void CTask::SetTaskNo(int jobno)
{
	m_taskNo = jobno;
}

//设置或获取任务名称
char* CTask::GetTaskName(void) const
{
	return (char *)m_taskName.c_str();
}

////获取或设置工作线程
//WorkerThread *  CTask::GetWorkThread()
//{
//	return m_pWorkThread;
//}
//void CTask::SetWorkThread(WorkerThread *  pWorkThread)
//{
//	m_pWorkThread = pWorkThread;
//}
//void CTask::DeleteWorkThread()
//{
//	m_pWorkThread = NULL;
//}
