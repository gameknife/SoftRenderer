#include "stdafx.h"
#include "SrRasTaskDispatcher.h"
#include "SrProfiler.h"

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
 *@brief �̵߳����к���
 *@return int 
 *@remark û�б�SetReadyʱ��һֱwait��ֱ����SetReady��
 ��ʼ�������񣬴�����ɺ�runningFlag��λ��֪ͨ�ⲿ������ɣ�ͬʱ����wait״̬
 */
int SrTaskThread::Run()
{
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
				task = m_creator->RequestTask();
			}
			
			if (!task)
			{
				// 全部任务完成
				break;
			}

			gEnv->profiler->setIncrement( (EProfilerElement)(ePe_Thread0TaskNum + m_threadId) );
			// ִ������
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
 *@brief ��ҵ�߳���ȡ����Ľӿڣ������ٽ�����֤������ȡ��ԭ���ԡ�
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
 *@brief ��ʼ������ַ���
 *@return void 
 *@remark ����cpu������������cpu������-1����ҵ�߳�
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

	m_resLock = new gkScopedLock<gkMutexLock>(eLGID_Resource, (uint64)this);

	m_preTaskToken = 0;
}

/**
 *@brief �ر�����ַ������ر���ҵ�߳�
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
 *@brief Э��flushģʽ�����̺߳������߳�һ����taskջ������
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
			// û�������ˣ��˳��߳�
			break;
		}
		gEnv->profiler->setIncrement(ePe_MainThreadTaskNum);

		// ִ������
		task->Execute();
	}
}

/**
 *@brief ����flushģʽ��֪ͨ�߳̿�ʼ����
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
 *@brief ���̵߳ȴ������߳���ɹ�����
 *@return void 
 */
void SrRasTaskDispatcher::Wait()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		// std::vector<HANDLE> handles;
		// SrTaskThreadPool::iterator it = m_pool.begin();
		// for ( ; it != m_pool.end(); ++it)
		// {
		// 	handles.push_back( (*it)->getWaitingHandle() );
		// }
  //
		// ::WaitForMultipleObjects(handles.size(), &(handles[0]), TRUE, INFINITE);


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

/**
 *@brief ���߳�������ַ�����ѹ������
 *@return void 
 *@param SrRasTask * task 
 */
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
