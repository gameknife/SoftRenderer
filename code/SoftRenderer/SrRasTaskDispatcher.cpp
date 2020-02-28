#include "stdafx.h"
#include "SrRasTaskDispatcher.h"
#include "SrProfiler.h"

#define		MAKEAFFINITYMASK1(x)					(1i64<<x)

SrTaskThread::SrTaskThread( int tileId, SrRasTaskDispatcher* creator ):m_creator(creator),
	m_threadId(tileId)
{
	m_runningFlag = new CManualResetEvent(false);
	m_waitFlag = new CManualResetEvent(false);
}

SrTaskThread::~SrTaskThread()
{
	delete m_runningFlag;
	delete m_waitFlag;
}

int SrTaskThread::Run()
{
#ifdef OS_WIN32
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)MAKEAFFINITYMASK1(m_threadId));
#endif
	
	while(true)
	{
		m_waitFlag->Wait();
		m_waitFlag->Reset();

		while(true)
		{
			SrRasTask* task = nullptr;
			if(!m_internalTasks.empty())
			{
				task = m_internalTasks.top();
				m_internalTasks.pop();
			}

			if(!task)
			{
				// 目前dispatcher里面已经不会有任务了
				// task = m_creator->RequestTask();
			}
			
			if (!task)
			{
				// 全部任务完成
				break;
			}

			gEnv->profiler->setIncrement( (EProfilerElement)(ePe_Thread0TaskNum + m_threadId) );
			// ִ执行任务
			task->Execute();
		}
		m_runningFlag->Set();
	}
	return 0;	
}

SrRasTaskDispatcher::SrRasTaskDispatcher(void)
{
	
}


SrRasTaskDispatcher::~SrRasTaskDispatcher(void)
{
}

SrRasTask* SrRasTaskDispatcher::RequestTask()
{
	SrRasTask* ret =  NULL;

	m_resLock->Lock();
	if (!m_taskStack.empty())
	{
		ret = m_taskStack.top();
		m_taskStack.pop();
	}
	m_resLock->UnLock();

	return ret;
}

void SrRasTaskDispatcher::Init()
{
	int threadCount = g_context->processorNum;

	for (int i=0; i < threadCount; ++i)
	{
		m_pool.push_back(new SrTaskThread(i, this));
	}
	
	SrTaskThreadPool::iterator it = m_pool.begin();
	for ( ; it != m_pool.end(); ++it)
	{
		(*it)->Start();
	}

	m_resLock = new gkScopedLock<gkMutexLock>(eLGID_Resource, (uint64)this);

	m_preTaskToken = 0;
}

void SrRasTaskDispatcher::Destroy()
{
	SAFE_DELETE(m_resLock);

	SrTaskThreadPool::iterator it = m_pool.begin();
	for ( ; it != m_pool.end(); ++it)
	{
		(*it)->Terminate();
		delete (*it);
	}
}

void SrRasTaskDispatcher::Flush()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			(*it)->SetReady();
		}
	}
}


void SrRasTaskDispatcher::Wait()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			(*it)->getWaitingEvent()->Wait();
		}
	}

	// destroy tasks
	SrTaskList::iterator itList = m_taskList.begin();
	for ( ; itList != m_taskList.end(); ++itList)
	{
		delete (*itList);
	}
	m_taskList.clear();

}

void SrRasTaskDispatcher::PushTask( SrRasTask* task )
{
	m_taskStack.push(task);
	m_taskList.push_back(task);
}

void SrRasTaskDispatcher::PrePushTask(SrRasTask* task)
{
	uint32 currWorker = m_preTaskToken++ % m_pool.size();
	m_pool[currWorker]->PrePushTask(task);
}
