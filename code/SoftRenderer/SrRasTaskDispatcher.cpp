#include "StdAfx.h"
#include "SrRasTaskDispatcher.h"
#include "SrProfiler.h"

#include "mmgr.h"

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

/**
 *@brief 线程的运行函数
 *@return int 
 *@remark 没有被SetReady时会一直wait，直到被SetReady后，
 开始处理任务，处理完成后将runningFlag置位，通知外部处理完成，同时进入wait状态
 */
int SrTaskThread::Run()
{
	while(true)
	{
		m_waitFlag->Wait();
		m_waitFlag->Reset();

		while(true)
		{
			SrRasTask* task = m_creator->RequestTask();
			if (!task)
			{
				// 没有任务了，退出线程
				break;
			}

			gEnv->profiler->setIncrement( (EProfilerElement)(ePe_Thread0TaskNum + m_threadId) );
			// 执行任务
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


/**
 *@brief 作业线程索取任务的接口，利用临界区保证任务索取的原子性。
 *@return SrRasTask* 
 */
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

/**
 *@brief 初始化任务分发器
 *@return void 
 *@remark 根据cpu核心数，开启cpu核心数-1个作业线程
 */
void SrRasTaskDispatcher::Init()
{
	int threadCount = g_context->processorNum - 1;

	for (int i=0; i < threadCount; ++i)
	{
		m_pool.push_back(new SrTaskThread(i, this));
	}
	
	SrTaskThreadPool::iterator it = m_pool.begin();
	for ( ; it != m_pool.end(); ++it)
	{
		(*it)->Start();
	}

	m_resLock = new gkScopedLock<gkMutexLock>(eLGID_Resource, (uint32)this);
}

/**
 *@brief 关闭任务分发器，关闭作业线程
 *@return void 
 */
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

/**
 *@brief 协作flush模式，主线程和其他线程一起处理task栈中任务
 *@return void 
 */
void SrRasTaskDispatcher::FlushCoop()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			(*it)->SetReady();
		}
	}
	
	// main thread also begin
	while(true)
	{
		SrRasTask* task = RequestTask();
		if (!task)
		{
			// 没有任务了，退出线程
			break;
		}
		gEnv->profiler->setIncrement(ePe_MainThreadTaskNum);

		// 执行任务
		task->Execute();
	}
}

/**
 *@brief 独立flush模式，通知线程开始处理。
 *@return void 
 */
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

/**
 *@brief 主线程等待所有线程完成工作。
 *@return void 
 */
void SrRasTaskDispatcher::Wait()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		std::vector<HANDLE> handles;
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			handles.push_back( (*it)->getWaitingHandle() );
		}

		::WaitForMultipleObjects(handles.size(), &(handles[0]), TRUE, INFINITE);
	}

	// destroy tasks
	SrTaskList::iterator itList = m_taskList.begin();
	for ( ; itList != m_taskList.end(); ++itList)
	{
		delete (*itList);
	}
	m_taskList.clear();

}

/**
 *@brief 主线程向任务分发器中压入任务
 *@return void 
 *@param SrRasTask * task 
 */
void SrRasTaskDispatcher::PushTask( SrRasTask* task )
{
	m_taskStack.push(task);
	m_taskList.push_back(task);
}
